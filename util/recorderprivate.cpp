#include <iostream>
#include <malloc.h>
#include <cstring>

#include "recorderprivate.h"
#include "util/persist.h"
#include "util/gameutils.h"

// In a spirit of friendly memory consumption stewardship, this class grows its recording buffer as needed in chunks of size:
constexpr int AllocationChunkSize = 1000;

RecorderActiveSource::RecorderActiveSource( RecorderPrivate& recorder ) : RecorderSource(recorder)
{
}

RecorderPersistedSource::RecorderPersistedSource( RecorderPrivate& recorder, Persist& persist ) : RecorderSource(recorder), mPersist(persist),
  mLoader(nullptr), mLoadSequence(0)
{
}

RecorderPersistedSource::~RecorderPersistedSource()
{
    delete mLoader;
}

RecorderSource::ReadState RecorderActiveSource::getReadState() const
{
    if ( mOffset < getCount() ) {
        return Ready;
    }
    return Finished;
}

RecorderSource::ReadState RecorderPersistedSource::getReadState() const
{
    if ( mOffset < getCount() ) {
        return Ready;
    }

    switch( mLoadSequence ) {
    case 0:
        if ( !mOffset && mPersist.isPersisted( mRecorder.getLevel() ) ) {
            return Pending;
        }
        return Finished;

    case 1:
        return Pending;

    default:
        return Finished;
    }
}

int RecorderActiveSource::getCount() const
{
    return mRecorder.getAvailableCount();
}

int RecorderPersistedSource::getCount() const
{
    return mRecorder.getPreRecordedCount();
}

unsigned char RecorderActiveSource::get()
{
    if ( mOffset < getCount() ) {
        return mRecorder.mRecorded[mOffset++].u.value;
    }

    return 0;
}

unsigned char RecorderPersistedSource::get()
{
    if ( mOffset < getCount() ) {
        return mRecorder.mRecorded[mOffset++].u.value;
    }

    if ( mLoadSequence == 0 && mLoader == nullptr ) {
        if ( (mLoader = mPersist.getLevelLoader( mRecorder.getLevel()) ) ) {
            if ( mRecorder.setData( *mLoader ) ) {
                connectDataReady( mLoader, SIGNAL(dataReady()) );
                mLoadSequence = 1;
                return 0;
            }
        }
        mLoadSequence = -1;
        if ( mLoader ) {
            delete mLoader;
            mLoader = nullptr;
        }
    }

    return 0;
}

void RecorderActiveSource::doDataReady()
{
    // do nothing
}

void RecorderPersistedSource::doDataReady()
{
    mLoadSequence = 2;
    if ( mLoader ) {
        delete mLoader;
        mLoader = nullptr;
    }
}

RecorderPrivate::RecorderPrivate( int capacity ) : mCapacity(capacity), mLevel{0}, mCurMove{}, mCurContinuation{}, mWritePos(0),
  mPreRecordedCount{0}
{
    // Allocate the initial chunk of the recording buffer
    //
    mRecordedAllocationWaterMark = std::min( capacity, AllocationChunkSize ) ;
    // Note +3 is to allow for lazy overflow detection; +3 handles worst case where last and previous both have continuations
    mRecorded = static_cast<EncodedMove*>( std::malloc( static_cast<size_t>(mRecordedAllocationWaterMark+3) * (sizeof *mRecorded) ) );
    if ( !mRecorded ) {
        std::cout << "** failed to allocate Recorder buffer!" << std::endl;
        // inhibit recording functionality by:
        mRecordedAllocationWaterMark = 0;
    }
}

RecorderPrivate::~RecorderPrivate()
{
    std::free( mRecorded );
}

void RecorderPrivate::onBoardLoaded( int level )
{
    if ( level == mLevel ) {
        // non-destructive rewind
        lazyFlush();
        mPreRecordedCount = mWritePos;
    } else {
        // reset
        mLevel = level;
        mPreRecordedCount = 0;
    }
    mCurMove.u.value = 0;
    mWritePos = 0;
}

bool RecorderPrivate::isEmpty() const
{
    return mCurMove.isEmpty() && !mWritePos && !getPreRecordedCount();
}

int RecorderPrivate::getAvailableCount() const
{
    if ( int count = getPreRecordedCount() ) {
        if ( !mCurMove.isEmpty() ) {
            if ( count <= mWritePos || !mCurMove.equals(mRecorded[mWritePos])  )
                return mWritePos+1;
            if ( !mCurContinuation.isEmpty() && mCurMove.u.move.shotCount == MAX_MOVE_SHOT_COUNT ) {
                if ( count <= mWritePos+1 || !mCurContinuation.equals(mRecorded[mWritePos+1]) )
                    return mWritePos+2;
            }
        }

        return count;
    }

    return getRecordedCount();
}

int RecorderPrivate::getRecordedCount() const
{
    int count = mWritePos;
    if ( !mCurMove.isEmpty() ) {
        ++count;
        if ( mCurMove.u.move.shotCount == MAX_MOVE_SHOT_COUNT && !mCurContinuation.isEmpty() ) {
            ++count;
        }
    }
    return count;
}

constexpr int PrintCount = 8;

int RecorderPrivate::storeInternal( EncodedMove move, int pos )
{
    if ( mPreRecordedCount ) {
        if ( pos < mPreRecordedCount && mRecorded[pos].u.value == move.u.value ) {
            return pos+1;
        }
        int start = std::max(pos-(PrintCount/2),0);
        dumpMoves( "original", &mRecorded[start], std::min(PrintCount, mPreRecordedCount-start), PrintCount/2 );
        mPreRecordedCount = 0;

        mRecorded[pos] = move;
        dumpMoves( "diverged", &mRecorded[start], std::min(PrintCount+1,pos-start+1) );
    }

    mRecorded[pos] = move;
    return pos+1;
}

void RecorderPrivate::lazyFlush()
{
    // flush any lazy write
    if ( !mCurMove.isEmpty() ) {
        mWritePos = storeInternal( mCurMove, mWritePos );
        if ( mCurMove.u.move.shotCount == MAX_MOVE_SHOT_COUNT && !mCurContinuation.isEmpty() ) {
            mWritePos = storeInternal( mCurContinuation, mWritePos );
        }
        mCurMove.clear();
    }
}

void RecorderPrivate::recordMove( bool adjacent, int rotation )
{
    // Starting a new move so finish the outstanding lazy write now if pending
    if ( !mCurMove.isEmpty() ) {
        if ( !commitCurMove() ) {
            return;
        }
        mCurMove.clear();
    }

    if ( adjacent ) {
        mCurMove.u.move.adjacent = 1;
    }

    switch( rotation ) {
    case   0: mCurMove.u.move.encodedAngle = 0; mCurMove.u.move.rotate = 1; break;
    case  90: mCurMove.u.move.encodedAngle = 1; mCurMove.u.move.rotate = 1; break;
    case 180: mCurMove.u.move.encodedAngle = 2; mCurMove.u.move.rotate = 1; break;
    case 270: mCurMove.u.move.encodedAngle = 3; mCurMove.u.move.rotate = 1; break;
    default:
        // caller isn't respecting our interface?
        std::cout << "** RecordRotation: direction not in -1,0,90,180,270: " << rotation << std::endl;
        // fall through
        [[clang::fallthrough]];
    case -1:
        mCurMove.u.move.rotate = 0;
    }
}

void RecorderPrivate::recordShot()
{
    if ( mCurMove.u.move.shotCount < MAX_MOVE_SHOT_COUNT ) {
        if ( ++mCurMove.u.move.shotCount == MAX_MOVE_SHOT_COUNT ) {
            // lazy clear
            mCurContinuation.clear();
        }
    } else if ( mCurContinuation.u.continuation.shotCount < MAX_CONTINUATION_SHOT_COUNT ) {
        ++mCurContinuation.u.continuation.shotCount;
    } else {
        // The most likely anticipated reason for reaching here would be for a board with more adjacent wooden
        // squares that the user wants to destroy all at once than can be achieved with
        // (MAX_MOVE_SHOT_COUNT+MAX_CONTINUATION_SHOT_COUNT)/2 total shots. Given the improbability,
        // handle this case by logging & ignoring:
        std::cout << "** recordShot: capping excessive shot count " << (MAX_MOVE_SHOT_COUNT+MAX_CONTINUATION_SHOT_COUNT) << std::endl;
    }
}

bool RecorderPrivate::ensureCapacity( int count )
{
    if ( count <= mCapacity ) {
        if ( count > mRecordedAllocationWaterMark ) {
            if ( void* p = std::realloc( mRecorded, count+3 ) ) {
                mRecorded = (EncodedMove*) p;
                mRecordedAllocationWaterMark = count;
            } else {
                return false;
            }
        }
    }
    return true;
}

bool RecorderPrivate::setData( PersistLevelLoader& loader )
{
    if ( int count = loader.getCount() ) {
        if ( ensureCapacity( count ) ) {
            if ( loader.load( *this ) ) {
                return true;
            }
        }
    }
    return false;
}

int RecorderPrivate::getLevel() const
{
    return mLevel;
}

void RecorderPrivate::dump()
{
    lazyFlush();
    hexDump( "recorder", mRecorded, mWritePos );
}

int RecorderPrivate::storeCurMove()
{
    int count = mWritePos;

    // confirm recording has not been inhibited
    if ( mRecordedAllocationWaterMark ) {

        // store unconditionally. Note this relies on mRecorded being sized +3.
        count = storeInternal( mCurMove, count );
        if ( mCurMove.u.move.shotCount == MAX_MOVE_SHOT_COUNT && !mCurContinuation.isEmpty() ) {
            count = storeInternal( mCurContinuation, count );
        }
    }

    return count;
}

bool RecorderPrivate::commitCurMove()
{
    int count = storeCurMove();
    if ( count >= mRecordedAllocationWaterMark ) {
        // check recording hasn't been inhibited
        if ( mRecordedAllocationWaterMark ) {

            //
            // attempt to grow the buffer
            //
            int newWaterMark = std::min( mRecordedAllocationWaterMark + AllocationChunkSize, mCapacity );
            if ( newWaterMark > mRecordedAllocationWaterMark ) {
                if ( void* p = std::realloc( mRecorded, (newWaterMark+3) * sizeof(EncodedMove) ) ) {
                    mRecorded = (EncodedMove*) p;
                    mRecordedAllocationWaterMark = newWaterMark;
                }
            }
        }

        // test for failure:
        if ( count >= mRecordedAllocationWaterMark ) {
            std::cout << "* Record buffer filled to capacity " << mCapacity << std::endl;
            return false;
        }
    }
    mWritePos = count;
    mCurMove.clear();
    return true;
}

int RecorderPrivate::getPreRecordedCount() const
{
    return mPreRecordedCount;
}

char* RecorderPrivate::getLoadableDestination( int forLevel, int count )
{
    if ( forLevel == mLevel ) {
        if ( count <= mRecordedAllocationWaterMark ) {
            return (char*) mRecorded;
        }
    }
    return nullptr;
}

void RecorderPrivate::releaseLoadableDestination( int forLevel, int actualCount )
{
    if ( forLevel == mLevel && !getRecordedCount() ) {
        mPreRecordedCount = std::max( actualCount, 0 );
        if ( mPreRecordedCount ) {
            mWritePos = 0;
            mCurMove.clear();
        }
    }
}

void RecorderPrivate::backdoor( int code )
{
    switch( code ) {
    case ('Q'<<8)|'M': // query moves
        std::cout << mWritePos << " written\n"
                  << mPreRecordedCount << " prerecorded\n";
        break;

    case ('D'<<8)|'M':
        if ( mPreRecordedCount != 0 ) {
            hexDump( "prerecorded", mRecorded, mPreRecordedCount );
        } else {
            hexDump( "written", mRecorded, mWritePos );
        }
        break;

    case ('L'<<8)|'M':
        if ( mPreRecordedCount != 0 ) {
            dumpMoves( "prerecorded", mRecorded, mPreRecordedCount, mWritePos );
        } else {
            dumpMoves( "written", mRecorded, mWritePos );
        }
        break;
    }
}

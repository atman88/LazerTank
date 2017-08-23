#include <iostream>
#include <malloc.h>
#include <cstring>

#include "recorderprivate.h"
#include "util/persist.h"

// In a spirit of friendly memory consumption stewardship, this class grows its recording buffer as needed in chunks of size:
#define ALLOCATION_CHUNK_SIZE 1000

RecorderSource::RecorderSource( RecorderPrivate& recorder, Persist& persist ) : QObject(0), mRecorder(recorder),
  mPersist(persist), mLoader(0), mOffset(0), mLoadSequence(0)
{
}

RecorderSource::~RecorderSource()
{
    if ( mLoader ) {
        delete mLoader;
    }
}

RecorderSource::ReadState RecorderSource::getReadState() const
{
    if ( mOffset < mRecorder.getCount() ) {
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

int RecorderSource::getCount()
{
    return mRecorder.getCount();
}

int RecorderSource::pos()
{
    return mOffset;
}

unsigned char RecorderSource::get()
{
    if ( mOffset < mRecorder.getCount() ) {
        return mRecorder.mRecorded[mOffset++].u.value;
    }

    if ( mLoadSequence == 0 && mLoader == 0 ) {
        if ( (mLoader = mPersist.getLevelLoader( mRecorder.getLevel()) ) ) {
            if ( mRecorder.setData( *mLoader ) ) {
                QObject::connect( mLoader, &PersistLevelLoader::dataReady, this, &RecorderSource::onDataReady );
                mLoadSequence = 1;
                return 0;
            }
        }
        mLoadSequence = 2;
        if ( mLoader ) {
            delete mLoader;
            mLoader = 0;
        }
    }

    return 0;
}

void RecorderSource::unget()
{
    if ( --mOffset < 0 ) {
        mOffset = 0;
    }
}

void RecorderSource::rewind()
{
    mOffset = 0;
}

int RecorderSource::seekEnd()
{
    mOffset = mRecorder.getCount();
    return mOffset;
}

void RecorderSource::onDataReady()
{
    disconnect( 0, 0, this, SIGNAL(onDataReady) );
    mLoadSequence = 2;
    if ( mLoader ) {
        delete mLoader;
        mLoader = 0;
    }
    emit dataReady();
}

RecorderPrivate::RecorderPrivate( int capacity ) : mLevel(0), mWritePos(0), mPreRecordedCount(0),
  mCapacity(capacity)
{
    mCurMove.clear();

    // Allocate the initial chunk of the recording buffer
    //
    mRecordedAllocationWaterMark = std::min( capacity, ALLOCATION_CHUNK_SIZE ) ;
    // Note +3 is to allow for lazy overflow detection; +3 handles worst case where last and previous both have continuations
    mRecorded = (EncodedMove*) std::malloc( (mRecordedAllocationWaterMark+3) * sizeof(EncodedMove) );
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
        mPreRecordedCount = mWritePos;
        mWritePos = 0;
    } else {
        // reset
        mLevel = level;
        mCurMove.u.value = 0;
        mWritePos = mPreRecordedCount = 0;
    }
}

bool RecorderPrivate::isEmpty() const
{
    return mCurMove.isEmpty() && !mWritePos && !mPreRecordedCount;
}

int RecorderPrivate::getCount() const
{
    if ( mPreRecordedCount ) {
        return mPreRecordedCount;
    }

    int count = mWritePos;
    if ( !mCurMove.isEmpty() ) {
        ++count;
        if ( mCurMove.u.move.shotCount == MAX_MOVE_SHOT_COUNT && !mCurContinuation.isEmpty() ) {
            ++count;
        }
    }
    return count;
}

int RecorderPrivate::storeInternal( EncodedMove move, int pos )
{
    if ( mPreRecordedCount ) {
        if ( pos < mPreRecordedCount && mRecorded[pos].u.value == move.u.value ) {
            return pos+1;
        }
        mPreRecordedCount = 0;
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
            int newWaterMark = std::min( mRecordedAllocationWaterMark + ALLOCATION_CHUNK_SIZE, mCapacity );
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
    return true;
}

char* RecorderPrivate::getLoadableDestination( int forLevel, int count )
{
    if ( forLevel == mLevel ) {
        if ( count <= mRecordedAllocationWaterMark ) {
            return (char*) mRecorded;
        }
    }
    return 0;
}

void RecorderPrivate::releaseLoadableDestination( int forLevel, int actualCount )
{
    if ( forLevel == mLevel ) {
        mWritePos = std::max( actualCount, 0 );
    }
}

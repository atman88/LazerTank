#include <iostream>
#include <malloc.h>
#include <cstring>

#include "recorderprivate.h"
#include "model/board.h"

// In a spirit of friendly memory consumption stewardship, this class grows its recording buffer as needed in chunks of size:
#define ALLOCATION_CHUNK_SIZE 1000

BufferSource::BufferSource( RecorderPrivate& recorder, QObject* parent ) : QObject(parent), mRecorder(recorder), mOffset(0)
{
}

BufferSource::ReadState BufferSource::getReadState()
{
    return (mOffset < mRecorder.getCount()) ? Ready : Finished;
}

int BufferSource::getCount()
{
    return mRecorder.getCount();
}

int BufferSource::pos()
{
    return mOffset;
}

unsigned char BufferSource::get()
{
    if ( mOffset < mRecorder.getCount() ) {
        return mRecorder.mRecorded[mOffset++].u.value;
    }

    // reached the end
    return 0;
}

void BufferSource::unget()
{
    if ( --mOffset < 0 ) {
        mOffset = 0;
    }
}

void BufferSource::rewind()
{
    mOffset = 0;
}

int BufferSource::seekEnd()
{
    mOffset = mRecorder.getCount();
    return mOffset;
}

RecorderPrivate::RecorderPrivate( int capacity ) : mLevel(0), mStartDirection(0), mWritePos(0), mPreRecordedCount(0),
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

void RecorderPrivate::onBoardLoaded( Board& board )
{
    if ( board.getLevel() == mLevel ) {
        // non-destructive rewind
        mPreRecordedCount = mWritePos;
        mWritePos = 0;
    } else {
        // reset
        mLevel = board.getLevel();
        mStartDirection = board.getTankStartVector().mAngle;
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

RecorderSource* RecorderPrivate::source()
{
    // flush any lazy write
    if ( !mCurMove.isEmpty() ) {
        mWritePos = storeInternal( mCurMove, mWritePos );
        if ( mCurMove.u.move.shotCount == MAX_MOVE_SHOT_COUNT && !mCurContinuation.isEmpty() ) {
            mWritePos = storeInternal( mCurContinuation, mWritePos );
        }
        mCurMove.clear();
    }
    return new BufferSource(*this);
}

RecorderReader* RecorderPrivate::getReader()
{
    return new RecorderReader( mStartDirection, *source() );
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

int RecorderPrivate::getData( unsigned char *data )
{
    int count = mWritePos;
    std::memcpy( data, mRecorded, count );
    if ( !mCurMove.isEmpty() ) {
        data[count++] = mCurMove.u.value;
        if ( mCurMove.u.move.shotCount == MAX_MOVE_SHOT_COUNT && !mCurContinuation.isEmpty() ) {
            data[count++] = mCurContinuation.u.value;
        }
    }
    return count;
}

bool RecorderPrivate::setData( int count, const unsigned char* data )
{
    if ( count <= mCapacity ) {
        if ( count > mRecordedAllocationWaterMark ) {
            if ( void* p = std::realloc( mRecorded, count ) ) {
                mRecorded = (EncodedMove*) p;
                mRecordedAllocationWaterMark = (count-3) / sizeof(EncodedMove);
            } else {
                return false;
            }
        }
        std::memcpy( mRecorded, data, count );
        mWritePos = count;
        mPreRecordedCount = 0;
        mCurMove.clear();
    }
    return true;
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

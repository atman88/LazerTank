#include <iostream>
#include <malloc.h>
#include "recorderprivate.h"

// In a spirit of friendly memory consumption stewardship, this class grows its recording buffer as needed in chunks of size:
#define ALLOCATION_CHUNK_SIZE 1000

RecorderPrivate::RecorderPrivate( int capacity ) : mStartDirection(0), mRecordedCount(0), mCapacity(capacity), mReader(0)
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
    if ( mReader ) {
        delete mReader;
    }
    std::free( mRecorded );
}

void RecorderPrivate::onBoardLoaded( int initialDirection )
{
    mStartDirection = initialDirection;
    // reset if not playing back
    if ( !mReader ) {
        mCurMove.u.value = 0;
        mRecordedCount = 0;
    }
}

bool RecorderPrivate::isEmpty() const
{
    return mCurMove.isEmpty() && mRecordedCount == 0;
}

int RecorderPrivate::getCount() const
{
    int count = mRecordedCount;
    if ( !mCurMove.isEmpty() ) {
        ++count;
        if ( mCurMove.u.move.shotCount == MAX_MOVE_SHOT_COUNT && !mCurContinuation.isEmpty() ) {
            ++count;
        }
    }
    return count;
}

RecorderReader*RecorderPrivate::getReader()
{
    if ( !mReader ) {
        mReader = new RecorderReader( this );
    }
    return mReader;
}

void RecorderPrivate::closeReader()
{
    if ( mReader ) {
        // Update our state so that recording resumes where the reader stopped:
        mRecordedCount = mReader->getOffset();
        mCurMove.clear(); // discard any lazy commit

        delete mReader;
        mReader = 0;
    }
}

void RecorderPrivate::recordMove(bool adjacent, int rotation)
{
    // inhibit writes when reading
    if ( mReader ) {
        return;
    }

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
    // inhibit writes when reading
    if ( mReader ) {
        return;
    }

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

int RecorderPrivate::storeCurMove()
{
    int count = mRecordedCount;

    // confirm recording has not been inhibited
    if ( mRecordedAllocationWaterMark ) {

        // store unconditionally. Note this relies on mRecorded being sized +3.
        mRecorded[count++] = mCurMove;
        if ( mCurMove.u.move.shotCount == MAX_MOVE_SHOT_COUNT && !mCurContinuation.isEmpty() ) {
            mRecorded[count++] = mCurContinuation;
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
    mRecordedCount = count;
    return true;
}

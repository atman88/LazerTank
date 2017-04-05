#include <iostream>
#include <malloc.h>
#include "recorderprivate.h"


RecorderPrivate::RecorderPrivate(int capacity) : mRecordedCount(0), mCapacity(capacity), mReader(0)
{
    mCurMove.clear();
    // allocate an additional two elements to allow for the lazily committed current value
    mRecorded = (EncodedMove*) malloc( (capacity+2) * sizeof(EncodedMove) );
}

RecorderPrivate::~RecorderPrivate()
{
    if ( mReader ) {
        delete mReader;
    }
    free( mRecorded );
}

void RecorderPrivate::reset()
{
    mCurMove.u.value = 0;
    mRecordedCount = 0;
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
        delete mReader;
        mReader = 0;
    }
}

void RecorderPrivate::recordMove(bool adjacent, int rotation)
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
    } else if ( mCurMove.u.continuation.shotCount < MAX_CONTINUATION_SHOT_COUNT ) {
        ++mCurContinuation.u.continuation.shotCount;
    } else {
        // The most likely anticipated reason for reaching here would be for a board with more adjacent wooden
        // squares that the user wants to destroy all at once than can be achieved with
        // (MAX_MOVE_SHOT_COUNT+MAX_CONTINUATION_SHOT_COUNT)/2 total shots. Given the improbability,
        // handle this case by logging & ignoring:
        std::cout << "** recordShot: capping excessive shot count " << (MAX_MOVE_SHOT_COUNT+MAX_CONTINUATION_SHOT_COUNT) << std::endl;
    }
}

bool RecorderPrivate::commitCurMove()
{
    if ( mRecordedCount >= mCapacity ) {
        std::cout << "* todo: level lost due to max # moves exceeded" << std::endl;
        return false;
    }
    mRecorded[mRecordedCount++] = mCurMove;
    if ( mCurMove.u.move.shotCount == MAX_MOVE_SHOT_COUNT && !mCurContinuation.isEmpty() ) {
        mRecorded[mRecordedCount++] = mCurContinuation;
    }
    return true;
}

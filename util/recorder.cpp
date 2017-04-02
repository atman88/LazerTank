#include <iostream>
#include "recorder.h"
#include "controller/game.h"

Recorder::~Recorder()
{
    if ( mReader ) {
        delete mReader;
        mReader = 0;
    }
}

void Recorder::reset()
{
    mCurMove.encodedAngle = 0;
    mCurMove.shotCount = 0;
    mRecordedCount = 0;
}

bool Recorder::isEmpty() const
{
    return mCurMove.encodedAngle == 0 && mCurMove.shotCount == 0 && mRecordedCount == 0;
}

int Recorder::getCount() const
{
    int count = mRecordedCount;
    if ( !isEmpty() ) {
        ++count;
    }
    return count;
}

RecorderReader* Recorder::getReader()
{
    if ( !mReader ) {
        mReader = new RecorderReader( this );
    }
    return mReader;
}

void Recorder::closeReader()
{
    if ( mReader ) {
        delete mReader;
        mReader = 0;
    }
}

void Recorder::addMove( int direction )
{
    if ( mCurMove.encodedAngle ) {
        if ( mRecordedCount == (sizeof mRecorded)/(sizeof *mRecorded) ) {
            std::cout << "* todo: level lost due to max # moves exceeded" << std::endl;
            return;
        }

        std::cout << "record " << ((int) mCurMove.encodedAngle) << "," << ((int) mCurMove.shotCount) << std::endl;
        mRecorded[mRecordedCount++] = mCurMove;
    }

    switch( direction ) {
    case   0: mCurMove.encodedAngle = 1; break;
    case  90: mCurMove.encodedAngle = 2; break;
    case 180: mCurMove.encodedAngle = 3; break;
    case 270: mCurMove.encodedAngle = 4; break;
    default:
        // this is unexpected, but rollback such that this call is a noop for this case
        --mRecordedCount;
        return;
    }
    mCurMove.shotCount = 0;
}

void Recorder::addShot()
{
    if ( mCurMove.shotCount < 31 ) {
        ++mCurMove.shotCount;
    }
}

RecorderReader::RecorderReader( Recorder *source ) : mOffset(0)
{
    mRecordedCount = source->mRecordedCount;
    mLastMove = source->mCurMove;
    mSource = source;
}

void RecorderReader::rewind()
{
    mOffset = 0;
}

bool RecorderReader::readNext( MoveController* controller )
{
    EncodedMove encoded;

    if ( mOffset < mRecordedCount ) {
        encoded = mSource->mRecorded[mOffset++];
    } else if ( mOffset == mRecordedCount ) {
        encoded = mLastMove;
        ++mOffset;
    } else {
        controller->setReplay( false );
        return false;
    }
    std::cout << "read " << ((int) encoded.encodedAngle) << "," << ((int) encoded.shotCount) << std::endl;

    int angle;
    switch( encoded.encodedAngle ) {
    case 1: angle =   0; break;
    case 2: angle =  90; break;
    case 3: angle = 180; break;
    case 4: angle = 270; break;
    default:
        std::cout << "**CORRUPT decoded angle at " << mOffset << std::endl;
        controller->setReplay( false );
        return false;
    }

    controller->move( angle );
    if ( encoded.shotCount ) {
        controller->fire( encoded.shotCount );
    }
    return true;
}

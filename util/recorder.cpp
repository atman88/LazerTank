#include <iostream>
#include "recorderprivate.h"
#include "controller/game.h"

Recorder::Recorder( int capacity )
{
    mPrivate = new RecorderPrivate( capacity );
}

Recorder::~Recorder()
{
    delete mPrivate;
}

void Recorder::onBoardLoaded()
{
    mPrivate->onBoardLoaded();
}

bool Recorder::isEmpty() const
{
    return mPrivate->isEmpty();
}

int Recorder::getCount() const
{
    return mPrivate->getCount();
}

RecorderReader* Recorder::getReader()
{
    return mPrivate->getReader();
}

void Recorder::closeReader()
{
    mPrivate->closeReader();
}

int Recorder::getCapacity() const
{
    return mPrivate->mCapacity;
}

void Recorder::recordMove( bool adjacent, int rotation )
{
    mPrivate->recordMove( adjacent, rotation );
}

void Recorder::recordShot()
{
    mPrivate->recordShot();
}

RecorderReader::RecorderReader( RecorderPrivate *source ) : mOffset(0), mLastDirection(0)
{
    mRecordedCount = source->storeCurMove();
    mSource = source;
}

void RecorderReader::rewind()
{
    mOffset = 0;
}

EncodedMove RecorderReader::readInternal()
{
    if ( mOffset < mRecordedCount ) {
        return mSource->mRecorded[mOffset++];
    }

    // reached the end
    EncodedMove empty;
    empty.clear();
    return empty;
}

int RecorderReader::getOffset() const
{
    return mOffset;
}

void RecorderReader::abort()
{
    // inhibit future reading by seeking to the end
    mOffset = mRecordedCount;
}

bool RecorderReader::readNext( RecorderConsumer* consumer )
{
    EncodedMove encoded = readInternal();
    if ( encoded.isEmpty() ) {
        // reached end
        consumer->setReplay( false );
        return false;
    }

    bool empty = true;
    if ( encoded.u.move.adjacent ) {
        consumer->move( mLastDirection, !encoded.u.move.rotate );
        empty = false;
    }

    if ( encoded.u.move.rotate ) {
        switch( encoded.u.move.encodedAngle ) {
        case 0: mLastDirection =   0; break;
        case 1: mLastDirection =  90; break;
        case 2: mLastDirection = 180; break;
        case 3: mLastDirection = 270; break;
        default:
            // this should be impossible given the bit field is two bits but let's see if the implementation proves that wrong
            std::cout << "**CORRUPT decoded angle at " << mOffset << std::endl;
            consumer->setReplay( false );
            abort();
            return false;
        }
        consumer->move( mLastDirection );
        empty = false;
    }

    // let's sanity-check while we're here:
    if ( empty ) {
        std::cout << "** readNext: Non-move read unexpectedly" << std::endl;
        abort();
        consumer->setReplay( false );
        return false;
    }

    if ( int shotCount = encoded.u.move.shotCount ) {
        if ( shotCount == MAX_MOVE_SHOT_COUNT ) {
            // check for a possible continuation
            encoded = readInternal();
            if ( encoded.u.continuation.header ) {
                // Not a continuation - undo this read (just peeking)
                --mOffset;
            } else {
                shotCount += encoded.u.continuation.shotCount;
                // Let's sanity-check while we're here:
                if ( shotCount == 7 ) {
                    std::cout << "**readNext: unexpected empty continuation encountered" << std::endl;
                    consumer->setReplay( false );
                    abort();
                    return false;
                }
            }
        }
        consumer->fire( shotCount );
    }
    return true;
}

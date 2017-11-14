#include <iostream>
#include "recorderprivate.h"
#include "persist.h"
#include "controller/gameregistry.h"

Recorder::Recorder( int capacity ) : QObject(0), mPrivate(new RecorderPrivate( capacity ))
{
}

Recorder::Recorder( RecorderPrivate* p ) : QObject(0), mPrivate(p)
{
}

Recorder::~Recorder()
{
    delete mPrivate;
}

void Recorder::onBoardLoaded( int level )
{
    mPrivate->onBoardLoaded( level );
    emit recordedCountChanged();
}

bool Recorder::isEmpty() const
{
    return mPrivate->isEmpty();
}

int Recorder::getAvailableCount() const
{
    return mPrivate->getAvailableCount();
}

int Recorder::getRecordedCount() const
{
    return mPrivate->getRecordedCount();
}

int Recorder::getLevel() const
{
    return mPrivate->getLevel();
}

int Recorder::getCapacity() const
{
    return mPrivate->mCapacity;
}

RecorderSource* Recorder::source()
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        mPrivate->lazyFlush();
        return new RecorderSource( *mPrivate, registry->getPersist() );
    }
    return 0;
}

void Recorder::recordMove( bool adjacent, int rotation )
{
    mPrivate->recordMove( adjacent, rotation );
    emit recordedCountChanged();
}

void Recorder::recordShot()
{
    mPrivate->recordShot();
}

RecorderReader::RecorderReader( int startDirection, RecorderSource& source ) : mLastDirection(startDirection), mSource(source)
{
}

RecorderReader::~RecorderReader()
{
    delete &mSource;
}

void RecorderReader::rewind()
{
    mSource.rewind();
}

int RecorderReader::pos() const
{
    return mSource.pos();
}

void RecorderReader::abort()
{
    // inhibit future reading by seeking to the end
    mSource.seekEnd();
}

bool RecorderReader::consumeNext( RecorderPlayer* player )
{
    EncodedMove encoded;

    encoded.u.value = mSource.get();
    if ( encoded.isEmpty() ) {
        if ( mSource.getReadState() == RecorderSource::Finished ) {
            player->setReplay( false );
            return false;
        }

        // pending
        return true;
    }

    bool empty = true;
    if ( encoded.u.move.adjacent ) {
        player->move( mLastDirection );
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
            std::cout << "**CORRUPT decoded angle at " << (mSource.pos()-1) << std::endl;
            player->setReplay( false );
            abort();
            return false;
        }

        player->move( mLastDirection );
        empty = false;
    }

    // let's sanity-check while we're here. Only the first move can fire only:
    if ( empty && mSource.pos() > 1 ) {
        std::cout << "** consumeNext: Non-move read unexpectedly" << std::endl;
        abort();
        player->setReplay( false );
        return false;
    }

    if ( int shotCount = encoded.u.move.shotCount ) {
        if ( shotCount == MAX_MOVE_SHOT_COUNT ) {
            // check for a possible continuation
            encoded.u.value = mSource.get();
            if ( encoded.u.continuation.header ) {
                // Not a continuation - undo this read (just peeking)
                mSource.unget();
            } else {
                shotCount += encoded.u.continuation.shotCount;
                // Let's sanity-check while we're here:
                if ( shotCount == 7 ) {
                    std::cout << "**consumeNext: unexpected empty continuation encountered" << std::endl;
                    player->setReplay( false );
                    abort();
                    return false;
                }
            }
        }
        player->fire( shotCount );
    }
    return true;
}

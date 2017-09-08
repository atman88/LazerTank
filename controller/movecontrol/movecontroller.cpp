#include <iostream>
#include "movecontroller.h"
#include "gameregistry.h"
#include "game.h"
#include "util/recorder.h"
#include "util/gameutils.h"

MoveController::MoveController(QObject *parent) : MoveDragController(parent), mReplayReader(0)
{
}

MoveController::~MoveController()
{
    if ( mReplayReader ) {
        delete mReplayReader;
    }
}

bool MoveController::canWakeup()
{
    if ( replaying() ) {
        return true;
    }
    return MoveDragController::canWakeup();
}

bool MoveController::setReplay( bool on )
{
    bool wasOn = (mReplayReader != 0);

    if ( on ) {
        if ( !mReplayReader ) {
            if ( GameRegistry* registry = getRegistry(this) ) {
                if ( RecorderSource* src = registry->getRecorder().source() ) {
                    if ( src->getReadState() == RecorderSource::Pending ) {
                        QObject::connect( src, &RecorderSource::dataReady, this, &MoveController::replayPlayback );
                    }
                    mReplayReader = new RecorderReader( registry->getGame().getBoard()->getTankStartVector().mAngle, *src );
                }
            }
            QObject::connect( this, &MoveController::idle, this, &MoveController::replayPlayback, Qt::QueuedConnection );
        }
    } else if ( mReplayReader ) {
        disconnect( this, SLOT(replayPlayback()) );
        delete mReplayReader;
        mReplayReader = 0;
        emit replayFinished();
    }

    return wasOn;
}

bool MoveController::replaying() const
{
    return mReplayReader != 0;
}

void MoveController::replayPlayback()
{
    if ( mReplayReader && mState == Idle ) {
        mReplayReader->consumeNext( this );
    }
}

int MoveDragController::getTileDragFocusAngle() const
{
    return mTileDragFocusAngle;
}

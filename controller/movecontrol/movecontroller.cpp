#include "movecontroller.h"
#include "gameregistry.h"
#include "model/tank.h"
#include "util/gameutils.h"

MoveController::MoveController(QObject *parent) : MoveDragController(parent), mReplaySource(0)
{
}

bool MoveController::canWakeup()
{
    if ( replaying() ) {
        return true;
    }
    return MoveDragController::canWakeup();
}

void MoveController::setReplay( bool on )
{
    if ( on ) {
        if ( !mReplaySource ) {
            if ( GameRegistry* registry = getRegistry(this) ) {
                mReplaySource = registry->getTank().getRecorder().getReader();
            }
            QObject::connect( this, &MoveController::idle, this, &MoveController::replayPlayback, Qt::QueuedConnection );
        }
    } else if ( mReplaySource ) {
        QObject::disconnect( this, &MoveController::idle, this, &MoveController::replayPlayback );
        if ( GameRegistry* registry = getRegistry(this) ) {
            registry->getTank().getRecorder().closeReader();
        }
        mReplaySource = 0;
        emit replayFinished();
    }
}

bool MoveController::replaying() const
{
    return mReplaySource != 0;
}

void MoveController::replayPlayback()
{
    if ( mReplaySource && mState == Idle ) {
        mReplaySource->readNext( this );
    }
}
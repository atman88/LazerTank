#include <iostream>
#include "movecontroller.h"
#include "gameregistry.h"
#include "util/recorder.h"
#include "util/gameutils.h"

MoveController::MoveController(QObject *parent) : MoveDragController(parent), mReplaySource(0)
{
}

MoveController::~MoveController()
{
    std::cout << "~MoveController mReplaySource=" << (mReplaySource ? "set" : "null") << std::endl;
    if ( mReplaySource ) {
        delete mReplaySource;
    }
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
                mReplaySource = registry->getRecorder().getReader();
            }
            QObject::connect( this, &MoveController::idle, this, &MoveController::replayPlayback, Qt::QueuedConnection );
        }
    } else if ( mReplaySource ) {
        QObject::disconnect( this, &MoveController::idle, this, &MoveController::replayPlayback );
        delete mReplaySource;
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
        mReplaySource->consumeNext( this );
    }
}

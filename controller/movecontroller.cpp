#include <iostream>
#include <QPainter>
#include "movecontroller.h"
#include "game.h"
#include "animationstateaggregator.h"
#include "pathsearchaction.h"
#include "model/tank.h"
#include "model/shotmodel.h"
#include "model/push.h"
#include "util/recorder.h"


MoveController::MoveController(QObject *parent) : QObject(parent), mState(Idle), mReplaySource(0)
{
}

void MoveController::init( GameRegistry* registry )
{
    setParent(registry);
    mFutureShots.setParent(registry);

    QObject::connect( &registry->getTank().getShot(), &ShotModel::shooterReleased,        this, &MoveController::wakeup, Qt::QueuedConnection );
    QObject::connect( &registry->getShotAggregate(), &AnimationStateAggregator::finished, this, &MoveController::wakeup, Qt::QueuedConnection );
}

void MoveController::onBoardLoaded( Board* board )
{
    mToVector = board->getTankStartVector();

    mFutureShots.reset();
    mMoves.reset();

    // emit unconditionally to prime the pump:
    mState = Idle;
    emit idle();
}

void MoveController::move( int direction, bool doWakeup )
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        Piece* lastMove = mMoves.getBack();
        ModelVector vector = (lastMove ? *lastMove : registry->getTank().getVector());

        if ( direction >= 0 && direction != vector.mAngle ) {
            if ( !lastMove || lastMove->hasPush() || lastMove->getShotCount()
               || (mState == FiringStage && mMoves.size() == 1) ) {
                appendMove( ModelVector(vector, direction) );
            } else {
                mMoves.replaceBack( MOVE_HIGHLIGHT, direction );
            }
        } else {
            Piece* pushPiece = 0;
            if ( registry->getGame().canMoveFrom( TANK, vector.mAngle, &vector, true, &pushPiece ) ) {
                appendMove( vector, pushPiece );
                if ( pushPiece ) {
                    registry->getGame().onFuturePush( pushPiece, vector.mAngle );
                }
            }
        }

        if ( doWakeup ) {
            wakeup();
        }
    }
}

void MoveController::fire( int count )
{
    if ( int nMoves = mMoves.size() ) {
        // -1 signifies an increment
        if ( count < 0 ) {
            count = mMoves.getBack()->getShotCount() + 1;
        }
        MovePiece* move = mMoves.setShotCountBack( count );
        // show its future if we expect it won't animate immediately:
//        if ( count > 1 || mState == RotateStage ) {
            mFutureShots.updatePath( move );
//        }
        if ( nMoves == 1 && mState == FiringStage ) {
            wakeup();
        }
    } else if ( count ) {
        if ( count < 0 ) {
            count = 1;
        }

        if ( GameRegistry* registry = getRegistry(this) ) {
            Piece* piece = mMoves.append( MOVE, registry->getTank().getVector(), count );
            // show it's future if it's got multiple shots
            if ( count > 1 ) {
                if ( MovePiece* move = dynamic_cast<MovePiece*>(piece) ) {
                    mFutureShots.updatePath( move );
                }
            }
            wakeup();
        }
    }
}

void MoveController::clearMoves()
{
    mMoves.reset();
}

void MoveController::wakeup()
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        Tank& tank = registry->getTank();

        // always wait while the trigger is pressed:
        if ( tank.getShot().getShooter() ) {
            return;
        }

        // always wait for any current rotation to complete before proceeding with a queued move:
        if ( mState == MovingStage && registry->getMoveAggregate().active() ) {
            return;
        }

        Piece* move = mMoves.getFront();
        while( move ) {
            if ( mState != FiringStage ) {
                if ( !tank.getVector().equals( *move ) ) {
                    bool hasRotation = mToVector.mAngle != move->getAngle();
                    mToVector = *move;

                    if ( move->hasPush() ) {
                        emit pushingInto( *move, move->getAngle() );
                    }

                    if ( tank.doMove( *move ) && (hasRotation || move->hasPush()) ) {
                        transitionState( MovingStage );
                        return; // waiting for current rotation to complete
                    }
                }
                transitionState( FiringStage );
            }

            if ( mState == FiringStage ) {
                if ( !move->getShotCount() ) {
                    if ( registry->getMoveAggregate().active() ) {
                        return; // waiting for current move to complete before advancing
                    }
                    transitionState( IdlingStage );
                } else {
                    if ( registry->getShotAggregate().active() ) {
                        return; // waiting for existing shot to complete
                    }
                    if ( tank.fire() ) {
                        if ( MovePiece* movePiece = dynamic_cast<MovePiece*>(move) ) {
                            movePiece->decrementShots();
                        }
                    }
                    return; // waiting for current shot to advance
                }
            }

            // nothing waiting on this move; advance to next move
            mFutureShots.removePath( move, false );
            mMoves.eraseFront();
            transitionState( IdlingStage );
            move = mMoves.getFront();
        }
    }

    transitionState( Idle );
}

void MoveController::eraseLastMove()
{
    if ( Piece* piece = mMoves.getBack() ) {
        mFutureShots.removePath( piece, true );
        mMoves.eraseBack();
        mMoves.replaceBack( MOVE_HIGHLIGHT );
    }
}

FutureShotPathManager& MoveController::getFutureShots()
{
    return mFutureShots;
}

PieceListManager& MoveController::getMoves()
{
    return mMoves;
}

void MoveController::onPathFound( PieceListManager* path, PathSearchAction* action )
{
    if ( action->getFocus() == TANK ) {
        mMoves.reset( path );
    } else {
        mMoves.replaceBack( MOVE );
        mMoves.append( path->getList() );
    }
    mMoves.replaceBack( MOVE_HIGHLIGHT );

    if ( action->getMoveWhenFound() ) {
        wakeup();
    }
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

void MoveController::appendMove( ModelVector vector, Piece* pushPiece )
{
    mMoves.replaceBack( MOVE ); // erase highlight
    mMoves.append( MOVE_HIGHLIGHT, vector, 0, pushPiece );
}

void MoveController::transitionState(MoveState newState)
{
    if ( newState != mState ) {
//        std::cout << "moveController ";
//        switch( newState ) {
//        case Idle:        std::cout << "Idle";        break;
//        case RotateStage: std::cout << "RotateStage"; break;
//        case FiringStage: std::cout << "FiringStage"; break;
//        case IdlingStage: std::cout << "IdlingStage"; break;
//        default:          std::cout << newState;      break;
//        }
//        std::cout << std::endl;

        mState = newState;
        if ( newState == Idle ) {
            emit idle();
        }
    }
}

void MoveController::setFocus( PieceType what )
{
    if ( mMoves.size() > 1 ) {
        mMoves.replaceBack( what == TANK ? MOVE : MOVE_HIGHLIGHT );
        mMoves.replaceFront(what == TANK ? MOVE_HIGHLIGHT : MOVE );
    }

    if ( GameRegistry* registry = getRegistry(this) ) {
        if ( what == TANK ) {
            registry->getTank().pause();
        } else {
            registry->getTank().resume();
        }
    }
}

void MoveController::replayPlayback()
{
    if ( mReplaySource && mState == Idle ) {
        mReplaySource->readNext( this );
    }
}

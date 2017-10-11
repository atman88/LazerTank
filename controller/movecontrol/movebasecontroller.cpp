#include <iostream>
#include "../movecontroller.h"
#include "../gameregistry.h"
#include "../game.h"
#include "../animationstateaggregator.h"
#include "../pathsearchaction.h"
#include "../model/tank.h"
#include "../model/shotmodel.h"
#include "../model/push.h"
#include "../model/board.h"


MoveBaseController::MoveBaseController( QObject* parent ) : QObject(parent), mState(Idle), mFocus(MOVE)
{
}

void MoveBaseController::init( GameRegistry* registry )
{
    setParent(registry);
    mFutureShots.setParent(registry);

    QObject::connect( &registry->getTank().getShot(), &ShotModel::shooterReleased,        this, &MoveController::wakeup, Qt::QueuedConnection );
    QObject::connect( &registry->getShotAggregate(), &AnimationStateAggregator::finished, this, &MoveController::wakeup, Qt::QueuedConnection );
}

void MoveBaseController::onBoardLoaded( Board& board )
{
    mToVector = board.getTankStartVector();

    mFutureShots.reset();
    mMoves.reset();

    // transition unconditionally to prime the pump:
    mState = Idle;
    onIdle();
}

void MoveBaseController::moveInternal( const ModelVector& origin, PieceListManager& moves, int direction, bool lastMoveBusy )
{
    Piece* lastMove = moves.getBack();
    ModelVector vector = (lastMove ? *lastMove : origin);

    if ( direction >= 0 && direction != vector.mAngle ) {
        if ( lastMoveBusy || !lastMove || lastMove->hasPush() || lastMove->getShotCount() ) {
            appendMove( moves, ModelVector(vector, direction) );
        } else {
            moves.replaceBack( MOVE_HIGHLIGHT, direction );
        }
    } else if ( GameRegistry* registry = getRegistry(this) ) {
        Piece* pushPiece = 0;
        if ( registry->getGame().canMoveFrom( TANK, vector.mAngle, &vector, true, &pushPiece ) ) {
            appendMove( moves, vector, pushPiece );
            if ( pushPiece ) {
                registry->getGame().onFuturePush( pushPiece, vector.mAngle );
            }
        }
    }
}

void MoveBaseController::move( int direction, bool doWakeup )
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        moveInternal( registry->getTank().getVector(), mMoves, direction, mState == FiringStage && mMoves.size() == 1 );

        if ( doWakeup ) {
            wakeup();
        }
    }
}

bool MoveBaseController::fireInternal( ModelVector initialVector, PieceListManager& moves, int count )
{
    if ( int nMoves = moves.size() ) {
        // -1 signifies an increment
        if ( count < 0 ) {
            count = moves.getBack()->getShotCount() + 1;
        }
        MovePiece* move = moves.setShotCountBack( count );
        mFutureShots.updatePath( move );
        if ( nMoves == 1 && mState == FiringStage ) {
            return true;
        }
    } else if ( count ) {
        if ( count < 0 ) {
            count = 1;
        }

        Piece* piece = moves.append( MOVE, initialVector, count );
        // show it's future if it's got multiple shots
        if ( count > 1 ) {
            if ( MovePiece* move = dynamic_cast<MovePiece*>(piece) ) {
                mFutureShots.updatePath( move );
            }
        }
        return true;
    }

    return true;
}

void MoveBaseController::fire( int count )
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        if ( fireInternal( registry->getTank().getVector(), mMoves, count ) ) {
            wakeup();
        }
    }
}

void MoveBaseController::undoLastMoveInternal( PieceListManager& moves )
{
    if ( Piece* piece = moves.getBack() ) {
        if ( piece->hasPush() ) {
            if ( GameRegistry* registry = getRegistry(this) ) {
                registry->getGame().undoFuturePush( dynamic_cast<MovePiece*>(piece) );
            }
        }
        mFutureShots.removePath( piece, true );
        moves.eraseBack();
    }
}

void MoveBaseController::undoLastMove()
{
    switch( mMoves.size() ) {
    case 0: // empty
        return;
    case 1: // allow if not doing this move
        switch( mState ) {
        case MovingStage:
        case FiringStage:
            return;
        default:
            ;
        }
    }

    undoLastMoveInternal( mMoves );
    mMoves.replaceBack( MOVE_HIGHLIGHT );
}

void MoveBaseController::undoMoves()
{
    while( mMoves.size() > 1 ) {
        undoLastMoveInternal( mMoves );
    }
    undoLastMove();
}

void MoveBaseController::wakeup()
{
    if ( !canWakeup() ) {
        return;
    }

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
                        registry->getGame().onTankPushingInto( *move, move->getAngle() );
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

FutureShotPathManager& MoveBaseController::getFutureShots()
{
    return mFutureShots;
}

bool MoveBaseController::canWakeup()
{
    return mFocus == MOVE;
}

PieceListManager& MoveBaseController::getMoves()
{
    return mMoves;
}

void MoveBaseController::onPathFound( PieceListManager* path, PathSearchCriteria* criteria )
{
    applyPathUsingCriteria( path, criteria );
    wakeup();
}

bool MoveBaseController::applyPathUsingCriteria( PieceListManager* path, PathSearchCriteria* criteria )
{
    if ( criteria->getFocus() == TANK ) {
        if ( GameRegistry* registry = getRegistry(this) ) {
            if ( !criteria->getStartPoint().equals( registry->getTank().getPoint() ) ) {
                std::cout << "* applyPathUsingCriteria: ingoring stale path" << std::endl;
                return false;
            }
            registry->getGame().endMoveDeltaTracking();
        } else {
            std::cout << "*** applyPathUsingCriteria: registry error" << std::endl;
            return false;
        }
        mMoves.reset( path );
    } else {
        if ( !criteria->getStartPoint().equals( getFocusVector() ) ) {
            std::cout << "* applyPathUsingCriteria: stale start point" << std::endl;
            return false;
        }
        mMoves.replaceBack( MOVE );
        mMoves.append( path->getList() );
    }
    mMoves.replaceBack( MOVE_HIGHLIGHT );
    return true;
}

void MoveBaseController::appendMove( PieceListManager& moves, ModelVector vector, Piece* pushPiece )
{
    moves.replaceBack( MOVE ); // erase highlight
    moves.append( MOVE_HIGHLIGHT, vector, 0, pushPiece );
}

void MoveBaseController::transitionState( MoveState newState )
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
            onIdle();
        }
    }
}

PieceType MoveBaseController::getFocus() const
{
    return mFocus;
}

void MoveBaseController::setFocus( PieceType what )
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        Tank& tank = registry->getTank();
        bool highlighted = false;

        if ( mMoves.size() ) {
            if ( mState == Idle ) {
                if ( Piece* front = mMoves.getFront() ) {
                    if ( !tank.getPoint().equals(*front) ) {
                        // This model is optimized - injecting a move for a proper visual:
                        mMoves.push_front( MOVE_HIGHLIGHT, tank.getVector() );
                        highlighted = true;
                    }
                }
            }
        }

        if ( mMoves.size() > 1 ) {
            mMoves.replaceBack( what == TANK ? MOVE : MOVE_HIGHLIGHT );
            if ( !highlighted ) {
                mMoves.replaceFront(what == TANK ? MOVE_HIGHLIGHT : MOVE );
            }
        }

        mFocus = what;

        if ( what == TANK ) {
            tank.pause();
        } else {
            tank.resume();
            wakeup();
        }
    }
}

ModelVector MoveBaseController::getFocusVector() const
{
    if ( mFocus == MOVE ) {
        if ( Piece* move = mMoves.getBack() ) {
            return *move;
        }
    } else if ( mState != Idle ) {
        if ( Piece* move = mMoves.getFront() ) {
            return *move;
        }
    }
    if ( GameRegistry* registry = getRegistry(this) ) {
        return registry->getTank().getVector();
    }
    return ModelVector();
}

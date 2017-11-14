#include <iostream>
#include <QAbstractEventDispatcher>

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
    mMoves.setParent(registry);

    QAbstractEventDispatcher *dispatcher = QAbstractEventDispatcher::instance();
    QObject::connect( dispatcher, &QAbstractEventDispatcher::aboutToBlock, this, &MoveBaseController::wakeup, Qt::DirectConnection );

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

void MoveBaseController::moveInternal( MoveListManager& moves, int direction )
{
    Piece* lastMove = moves.getBack();
    ModelVector vector = (lastMove ? *lastMove : moves.getInitialVector());

    if ( direction >= 0 && direction != vector.mAngle ) {
        if ( !lastMove || lastMove->hasPush() || lastMove->getShotCount() || (&moves == &mMoves && moves.size()==1 && mState >= FiringStage) ) {
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

void MoveBaseController::move( int direction )
{
    if ( mFocus == TANK ) {
        undoMoves();
        setFocus( MOVE );
    }
    moveInternal( mMoves, direction );
}

void MoveBaseController::fireInternal( MoveListManager& moves, int count )
{
    if ( int nMoves = moves.size() ) {
        bool moveInPlay = (&moves == &mMoves && nMoves == 1);
        if ( !moveInPlay || mState <= FiringStage ) {
            Piece* piece = moves.getBack();
            int previousCount = piece->getShotCount();
            // -1 signifies an increment
            if ( count < 0 ) {
                count = previousCount + 1;
            }
            MovePiece* move = moves.setShotCountBack( count );
            mFutureShots.updateShots( previousCount, move );
            return;
        }
    }

    if ( count ) {
        if ( count < 0 ) {
            count = 1;
        }

        MovePiece* move = moves.append( MOVE, moves.getInitialVector(), count );
        mFutureShots.updateShots( 0, move );
        return;
    }
}

void MoveBaseController::fire( int count )
{
    fireInternal( mMoves, count );
}

void MoveBaseController::undoLastMoveInternal( PieceListManager& moves )
{
    if ( Piece* piece = moves.getBack() ) {
        mFutureShots.removePath( piece, true );
        if ( piece->hasPush() ) {
            if ( GameRegistry* registry = getRegistry(this) ) {
                registry->getGame().undoFuturePush( dynamic_cast<MovePiece*>(piece) );
            }
        }
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

void MoveBaseController::undo()
{
    if ( mFocus != TANK ) {
        undoLastMove();
    } else {
        undoMoves();
        setFocus( MOVE );
    }
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
#ifndef QT_NO_DEBUG
                    } else {
                        if ( GameRegistry* registry = getRegistry(this) ) {
                            if ( registry->getGame().getBoard()->getPieceManager().pieceAt(*move) ) {
                                std::cout << "doMove: moving over piece" << std::endl;
                            }
                        }
#endif // QT_NO_DEBUG
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
}

bool MoveBaseController::applyPathUsingCriteria( PieceListManager* path, PathSearchCriteria* criteria )
{
    if ( criteria->getFocus() == TANK ) {
        if ( GameRegistry* registry = getRegistry(this) ) {
            if ( !criteria->getStartPoint().equals( registry->getTank().getPoint() ) ) {
                std::cout << "* applyPathUsingCriteria: ingoring stale path" << std::endl;
                return false;
            }
        } else {
            std::cout << "*** applyPathUsingCriteria: registry error" << std::endl;
            return false;
        }
        mMoves.reset( path );
    } else {
        if ( !criteria->getStartPoint().equals( getBaseFocusVector() ) ) {
            std::cout << "* applyPathUsingCriteria: stale start point" << std::endl;
            return false;
        }
        mMoves.replaceBack( MOVE );
        mMoves.appendList( path->getList() );
    }
    mMoves.replaceBack( MOVE_HIGHLIGHT );
    return true;
}

void MoveBaseController::appendMove( MoveListManager& moves, ModelVector vector, Piece* pushPiece )
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
//        case MovingStage: std::cout << "MovingStage"; break;
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
    }
}

ModelVector MoveBaseController::getBaseFocusVector( PieceType focus ) const
{
    if ( focus == NONE ) {
        focus = mFocus;
    }

    if ( focus == MOVE ) {
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
    std::cout << "getBaseFocus tank not registered" << std::endl;
    return ModelVector();
}

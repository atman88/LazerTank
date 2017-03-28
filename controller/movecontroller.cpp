#include "movecontroller.h"
#include "game.h"
#include "pathsearchaction.h"


MoveController::MoveController(QObject *parent) : QObject(parent), mIdle(false)
{
}

void MoveController::init( Game* game )
{
    setParent(game);
    mFutureShots.setParent(game);

    QObject::connect( &game->getTank()->getShot(), &ShotModel::shooterReleased,      this, &MoveController::wakeup, Qt::QueuedConnection );
    QObject::connect( game->getShotAggregate(), &AnimationStateAggregator::finished, this, &MoveController::wakeup, Qt::QueuedConnection );
}

void MoveController::reset()
{
    mToPoint.setNull();
    mToDirection = -1;

    mFutureShots.reset();
    mMoves.reset();
    mIdle = true;
}

void MoveController::move( int direction )
{
    if ( Game* game = getGame(this) ) {
        Tank* tank = game->getTank();

        if ( direction >= 0 ) {
            int fromRotation;
            if ( !mMoves.size() ) {
                fromRotation = tank->getRotation();
            } else {
                fromRotation = mMoves.getList()->back()->getAngle();
            }

            if ( direction != fromRotation ) {
                if ( !mMoves.size() ) {
                    mToPoint.mCol = tank->getCol();
                    mToPoint.mRow = tank->getRow();
                    mToDirection = direction;
                    appendMove( mToPoint.mCol, mToPoint.mRow, direction );
                    // changing direction only so do without delay
                    if ( tank->doMove( mToPoint.mCol, mToPoint.mRow, direction ) ) {
                        mIdle = false;
                    }
                    return;
                }

                Piece* move = mMoves.getList()->back();
                if ( move->hasPush() || move->getShotCount() ) {
                    appendMove( move->getCol(), move->getRow(), direction );
                } else {
                    mMoves.replaceBack( MOVE_HIGHLIGHT, direction );
                }
            } else {
                int col, row;
                if ( !mMoves.size() ) {
                    col = tank->getCol();
                    row = tank->getRow();
                } else {
                    Piece* last = mMoves.getList()->back();
                    col = last->getCol();
                    row = last->getRow();
                }

                Piece* pushPiece = 0;
                if ( game->canMoveFrom( TANK, direction, &col, &row, true, &pushPiece ) ) {
                    appendMove( col, row, direction, 0, pushPiece );
                    if ( pushPiece ) {
                        game->onFuturePush( pushPiece, direction );
                    }
                }
            }
        }

        // wake it up if not active
        wakeup();
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
        mFutureShots.updatePath( move );
        if ( nMoves == 1 ) {
            wakeup();
        }
    } else if ( count ) {
        if ( count < 0 ) {
            count = 1;
        }

        if ( Game* game = getGame(this) ) {
            Tank* tank = game->getTank();

            // try to fire now if not presently shooting
            if ( !game->getShotAggregate()->active() ) {
                if ( tank->fire() ) {
                    --count;
                    mIdle = false;
                }
            }

            if ( count > 0 ) {
                mMoves.append( MOVE, tank->getCol(), tank->getRow(), tank->getRotation(), count );
            }
        }
    }
}

void MoveController::clearMoves()
{
    mMoves.reset();
}

void MoveController::wakeup()
{
    Piece* move = mMoves.getFront();
    bool busy = false;

    if ( Game* game = getGame(this) ) {
        Tank* tank = game->getTank();

        if ( !(busy = tank->getShot().getShooter() != 0) ) {
            if ( !(busy = game->getMoveAggregate()->active()) ) {
                while( move ) {
                    if ( move->getCol() != tank->getCol() || move->getRow() != tank->getRow() || move->getAngle() != tank->getRotation() ) {
                        mToPoint.mCol = move->getCol();
                        mToPoint.mRow = move->getRow();
                        mToDirection = move->getAngle();

                        if ( move->hasPush() ) {
                            emit pushingInto( move->getCol(), move->getRow(), move->getAngle() );
                        }
                        busy = tank->doMove( move->getCol(), move->getRow(), move->getAngle() );
                        break;
                    }

                    if ( move->getShotCount() ) {
                        break;
                    }

                    mMoves.eraseFront();
                    move = mMoves.getFront();
                }
            }
        }

        if ( !busy && move ) {
            if ( !(busy = game->getShotAggregate()->active()) ) {
                if ( move->getAngle() == tank->getRotation() || !game->getMoveAggregate()->active() ) {
                    if ( MovePiece* movePiece = dynamic_cast<MovePiece*>(move) ) {
                        if ( movePiece->decrementShots() >= 0 ) {
                            busy = tank->fire();
                        }
                    }
                }
            }
        }
    }

    if ( busy || move ) {
        mIdle = false;
    } else if ( !mIdle ) {
        mIdle = true;
        emit idle();
    }
}

void MoveController::eraseLastMove()
{
    if ( Piece* piece = mMoves.getBack() ) {
        mFutureShots.removePath( piece );
        mMoves.eraseBack();
        mMoves.replaceBack( MOVE_HIGHLIGHT );
    }
}

FutureShotPathManager* MoveController::getFutureShots()
{
    return &mFutureShots;
}

PieceListManager* MoveController::getMoves()
{
    return &mMoves;
}

void MoveController::onPathFound( PieceListManager* path, PathSearchAction* action )
{
    if ( action->getFocus() == TANK ) {
        mMoves.reset( path );
    } else {
        mMoves.replaceBack( MOVE );
        mMoves.append( *path->getList() );
    }
    mMoves.replaceBack( MOVE_HIGHLIGHT );

    if ( action->getMoveWhenFound() ) {
        wakeup();
    }
}

void MoveController::appendMove( int col, int row, int direction, int shotCount, Piece* pushPiece )
{
    mMoves.replaceBack( MOVE ); // erase highlight
    mMoves.append( MOVE_HIGHLIGHT, col, row, direction, shotCount, pushPiece );
}

void MoveController::setFocus( PieceType what )
{
    if ( mMoves.size() > 1 ) {
        mMoves.replaceBack( what == TANK ? MOVE : MOVE_HIGHLIGHT );
        mMoves.replaceFront(what == TANK ? MOVE_HIGHLIGHT : MOVE );
    }

    if ( Game* game = getGame(this) ) {
        if ( what == TANK ) {
            game->getTank()->pause();
        } else {
            game->getTank()->resume();
        }
    }
}

#include <iostream>
#include <QEvent>
#include <QCoreApplication>
#include "tank.h"
#include "controller/game.h"
#include "controller/pathsearchaction.h"
#include "util/imageutils.h"
#include "util/renderutils.h"
#include "util/gameutils.h"

Tank::Tank(QObject* parent) : TankView(parent), mCol(0), mRow(0), mBusyFiring(false)
{
}

void Tank::init( Game* game )
{
    setParent(game);
    mFutureShots.setParent(game);
    AnimationStateAggregator* aggregate = game->getMoveAggregate();
    QObject::connect( &mRotateAnimation,     &QPropertyAnimation::stateChanged, aggregate, &AnimationStateAggregator::onStateChanged );
    QObject::connect( &mHorizontalAnimation, &QPropertyAnimation::stateChanged, aggregate, &AnimationStateAggregator::onStateChanged );
    QObject::connect( &mVerticalAnimation,   &QPropertyAnimation::stateChanged, aggregate, &AnimationStateAggregator::onStateChanged );
    QObject::connect( aggregate, &AnimationStateAggregator::finished, this, &Tank::onAnimationsFinished, Qt::QueuedConnection );
    QObject::connect( &getShot(), &ShotModel::shooterReleased, this, &Tank::wakeup );
    SpeedController* controller = game->getSpeedController();
    mRotateAnimation.setController( controller );
    mHorizontalAnimation.setController( controller );
    mVerticalAnimation.setController( controller );
    TankView::init( game );
}

void Tank::reset( int col, int row )
{
    mCol = col;
    mRow = row;
    mRotation = 0;
    mFutureShots.reset();
    mMoves.reset();
    mBusyFiring = false;
    QObject::disconnect( getGame(this)->getShotAggregate(), &AnimationStateAggregator::finished, this, &Tank::resumeMove );
    QPoint p( col*24, row*24 );
    TankView::reset( p );
}

void Tank::fire( int count )
{
    if ( int nMoves = mMoves.size() ) {
        // -1 signifies an increment
        if ( count < 0 ) {
            count = mMoves.getBack()->getShotCount() + 1;
        }
        if ( nMoves > 1 ) {
            MovePiece* move = mMoves.setShotCountBack( count );
            mFutureShots.updatePath( move );
            return;
        }
    }
    continueMove( count );
}

void Tank::clearMoves()
{
    mMoves.reset();
}

void Tank::wakeup()
{
    move(-1);
}

void Tank::appendMove( int col, int row, int direction, int shotCount, Piece* pushPiece )
{
    mMoves.replaceBack( MOVE ); // erase highlight
    mMoves.append( MOVE_HIGHLIGHT, col, row, direction, shotCount, pushPiece );
}

void Tank::move( int direction )
{
    Game* game = getGame(this);
    if ( !game ) {
        return;
    }

    if ( direction >= 0 ) {
        int fromRotation;
        if ( !mMoves.size() ) {
            fromRotation = mRotation;
        } else {
            fromRotation = mMoves.getList()->back()->getAngle();
        }
        if ( direction != fromRotation ) {
            if ( !mMoves.size() ) {
                appendMove( mCol, mRow, direction );
                // changing direction only so do without delay
                doMove( mCol, mRow, direction );
                return;
            }

            Piece* move = mMoves.getList()->back();
            if ( move->hasPush() || move->getShotCount() ) {
                appendMove( move->getCol(), move->getRow(), direction );
            } else {
                mMoves.replaceBack( MOVE_HIGHLIGHT, direction );
                if ( mMoves.size() == 1 ) {
                    // changing only direction of current move so do without delay
                    doMove( move->getCol(), move->getRow(), direction );
                    return;
                }
            }
        } else {
            int col, row;
            if ( !mMoves.size() ) {
                col = mCol;
                row = mRow;
            } else {
                Piece* last = mMoves.getList()->back();
                col = last->getCol();
                row = last->getRow();
            }

            Piece* pushPiece = 0;
            if ( game && game->canMoveFrom( TANK, direction, &col, &row, true, &pushPiece ) ) {
                appendMove( col, row, direction, 0, pushPiece );
                if ( pushPiece ) {
                    game->onFuturePush( pushPiece, direction );
                }
            }
        }
    }

    // wake it up if not active
    if ( !game->getMoveAggregate()->active() && !waitingOnShots() ) {
        if ( Piece* move = mMoves.getFront() ) {
            if ( move->hasPush() ) {
                emit movingInto( move->getCol(), move->getRow(), move->getAngle() );
            }
            doMove( move->getCol(), move->getRow(), move->getAngle() );
        } else {
            emit idle();
        }
    }
}

void Tank::doMove( int col, int row, int direction )
{
    bool doing = mRotateAnimation.animateBetween( mViewRotation, direction )
               | mHorizontalAnimation.animateBetween( getViewX().toInt(), col*24 )
               | mVerticalAnimation.animateBetween(   getViewY().toInt(), row*24 );
    if ( !doing ) {
        resumeMove();
    }
}

int Tank::getRow() const
{
    return mRow;
}

int Tank::getRotation() const
{
    return mRotation;
}

void Tank::eraseLastMove()
{
    if ( Piece* piece = mMoves.getBack() ) {
        mFutureShots.removePath( piece );
        mMoves.eraseBack();
        mMoves.replaceBack( MOVE_HIGHLIGHT );
    }
}

int Tank::getCol() const
{
    return mCol;
}

void Tank::onAnimationsFinished()
{
    if ( mMoves.size() && !mBusyFiring ) {
        resumeMove();
    }
}

void Tank::onMoved(int col, int row, int rotation)
{
    mCol = col;
    mRow = row;
    mRotation = rotation;
    resumeMove();
}

bool Tank::waitingOnShots()
{
    if ( mBusyFiring ) {
        return true;
    }
    if ( getShot().getShooter() ) {
        // delay until the shot detaches from us for the case where the subsequent move is changing direction
        // so that we remain trained on the current target to avoid our shot from displaying bent:
        if ( Piece* nextMove = mMoves.getFront() ) {
            return nextMove->getAngle() != mRotation;
        }
    }
    return false;
}

FutureShotPathManager* Tank::getFutureShots()
{
    return &mFutureShots;
}

void Tank::resumeMove()
{
    if ( Piece* move = mMoves.getFront() ) {
        // continue now if the tank's direction isn't changing or the tank isn't currently moving
        if ( mRotation != move->getAngle() ) {
            return;
        }
        if ( Game* game = getGame(this) ) {
            if ( game->getMoveAggregate()->active() ) {
                return;
            }
        }
        continueMove( move->getShotCount(), move );
    }
}

void Tank::continueMove( int shotCount, Piece* move )
{
    if ( shotCount ) {
        // determine if presently shooting
        bool busy = false;
        if ( Game* game = getGame(this) ) {
            busy = game->getShotAggregate()->active();
        }

        if ( !busy ) {
            Shooter::fire();
            if ( !move ) {
                --shotCount;
            } else {
                shotCount = dynamic_cast<MovePiece*>(move)->decrementShots();
            }
        }
        if ( shotCount > 0 ) {
            if ( !move ) {
                mMoves.append( MOVE, mCol, mRow, mRotation, shotCount );
            }
            if ( !mBusyFiring ) {
                mBusyFiring = true;
                QObject::connect( getGame(this)->getShotAggregate(), &AnimationStateAggregator::finished, this, &Tank::resumeMove );
            }
        }
    }

    if ( shotCount <= 0 ) {
        if ( move ) {
            mFutureShots.removePath( move );
            mMoves.eraseFront();
        }
        if ( mBusyFiring ) {
            QObject::disconnect( getGame(this)->getShotAggregate(), &AnimationStateAggregator::finished, this, &Tank::resumeMove );
            mBusyFiring = false;
        }
        wakeup();
    }
}

PieceListManager* Tank::getMoves()
{
    return &mMoves;
}

void Tank::setFocus( PieceType what )
{
    if ( mMoves.size() > 1 ) {
        mMoves.replaceBack( what == TANK ? MOVE : MOVE_HIGHLIGHT );
        mMoves.replaceFront(what == TANK ? MOVE_HIGHLIGHT : MOVE );
    }

    if ( what == TANK ) {
        pause();
    } else {
        resume();
    }
}

void Tank::onPathFound( PieceListManager* path, PathSearchAction* action )
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

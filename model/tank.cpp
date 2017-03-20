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
    AnimationStateAggregator* aggregate = game->getMoveAggregate();
    QObject::connect( &mRotateAnimation,     &QPropertyAnimation::stateChanged, aggregate, &AnimationStateAggregator::onStateChanged );
    QObject::connect( &mHorizontalAnimation, &QPropertyAnimation::stateChanged, aggregate, &AnimationStateAggregator::onStateChanged );
    QObject::connect( &mVerticalAnimation,   &QPropertyAnimation::stateChanged, aggregate, &AnimationStateAggregator::onStateChanged );
    QObject::connect( aggregate, &AnimationStateAggregator::finished, this, &Tank::onAnimationsFinished, Qt::QueuedConnection );

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
    mMoves.reset();
    mBusyFiring = false;
    QObject::disconnect( getGame(this)->getShotAggregate(), &AnimationStateAggregator::finished, this, &Tank::resumeMove );
    QPoint p( col*24, row*24 );
    TankView::reset( p );
}

void Tank::fire( int count )
{
    if ( !mMoves.size() ) {
        if ( count < 0 ) {
            Shooter::fire();
        } else if ( count > 0 ) {
            mMoves.append( MOVE, mCol, mRow, mRotation, count );
            resumeMove();
        }
    } else {
        if ( count < 0 ) {
            count = mMoves.getBack()->getShotCount() + 1;
        }
        mMoves.setShotCountBack( count );
        wakeup();
    }
}

void Tank::clearMoves()
{
    mMoves.reset();
}

void Tank::wakeup()
{
    move(-1);
}

void Tank::appendMove( int col, int row, int direction, Piece* pushPiece )
{
    mMoves.replaceBack( MOVE ); // erase highlight
    mMoves.append( MOVE_HIGHLIGHT, col, row, direction, pushPiece );
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
                appendMove( col, row, direction, pushPiece );
                if ( pushPiece ) {
                    game->onFuturePush( pushPiece, direction );
                }
            }
        }
    }

    // wake it up if not active
    if ( !game->getMoveAggregate()->active() && !mBusyFiring ) {
        if ( Piece* move = mMoves.getFront() ) {
            doMove( move->getCol(), move->getRow(), move->getAngle() );
            if ( move->hasPush() ) {
                emit movingInto( move->getCol(), move->getRow(), move->getAngle() );
            }
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
    mMoves.eraseBack();
    mMoves.replaceBack( MOVE_HIGHLIGHT );
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

void Tank::resumeMove()
{
    Piece* move = mMoves.getList()->front();
    if ( move && mCol == move->getCol() && mRow == move->getRow() && mRotation == move->getAngle() ) {
        if ( move->getShotCount() ) {
            // ensure we're not already busing shooting
            if ( Game* game = getGame(this) ) {
                if ( game->getShotAggregate()->active() ) {
                    return;
                }
            }
        }
        int shotCount = move->decrementShots();
        if ( shotCount >= 0 ) {
            Shooter::fire();
            if ( shotCount > 0 && !mBusyFiring ) {
                mBusyFiring = true;
                QObject::connect( getGame(this)->getShotAggregate(), &AnimationStateAggregator::finished, this, &Tank::resumeMove );
            }
        }
        if ( shotCount <= 0 ) {
            mMoves.eraseFront();
            if ( mBusyFiring ) {
                QObject::disconnect( getGame(this)->getShotAggregate(), &AnimationStateAggregator::finished, this, &Tank::resumeMove );
                mBusyFiring = false;
            }
            wakeup();
        }
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

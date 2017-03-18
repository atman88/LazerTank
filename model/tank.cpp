#include <iostream>
#include <QEvent>
#include <QCoreApplication>
#include "tank.h"
#include "controller/game.h"
#include "controller/pathsearchaction.h"
#include "util/imageutils.h"
#include "util/renderutils.h"
#include "util/gameutils.h"

Tank::Tank(QObject* parent) : TankView(parent), mCol(0), mRow(0)
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
    QPoint p( col*24, row*24 );
    TankView::reset( p );
}

void Tank::clearMoves()
{
    mMoves.reset();
}

void Tank::wakeup()
{
    move(-1);
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
                mMoves.append( MOVE, mCol, mRow, direction );
                // changing direction only so do without delay
                doMove( mCol, mRow, direction );
                return;
            }

            Piece* move = mMoves.getList()->back();
            if ( move->hasPush() ) {
                mMoves.append( MOVE, move->getCol(), move->getRow(), direction );
            } else {
                mMoves.replaceBack( MOVE, direction );
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
                mMoves.append( MOVE, col, row, direction, pushPiece );
                if ( pushPiece ) {
                    game->onFuturePush( pushPiece, direction );
                }
            }
        }
    }

    // wake it up if not active
    if ( mMoves.size() && !game->getMoveAggregate()->active() ) {
        Piece* move = mMoves.getList()->front();
        doMove( move->getCol(), move->getRow(), move->getAngle() );
        if ( move->hasPush() ) {
            emit movingInto( move->getCol(), move->getRow(), move->getAngle() );
        }
    }
}

void Tank::doMove( int col, int row, int direction )
{
    mRotateAnimation.animateBetween( mViewRotation, direction );
    mHorizontalAnimation.animateBetween( getViewX().toInt(), col*24 );
    mVerticalAnimation.animateBetween(   getViewY().toInt(), row*24 );
}

int Tank::getRow() const
{
    return mRow;
}

int Tank::getRotation() const
{
    return mRotation;
}

int Tank::getCol() const
{
    return mCol;
}

void Tank::onAnimationsFinished()
{
    if ( mMoves.size() ) {
        Piece* piece = mMoves.getList()->front();
        if ( piece->getAngle() == mRotation && piece->getCol() == mCol && piece->getRow() == mRow ) {
            mMoves.eraseFront();
        }
    }
}

void Tank::onMoved(int col, int row, int rotation)
{
    mCol = col;
    mRow = row;
    mRotation = rotation;
    Piece* move = mMoves.getList()->front();
    if ( move && col == move->getCol() && row == move->getRow() && rotation == move->getAngle() ) {
        mMoves.eraseFront();
    }
}

PieceListManager* Tank::getMoves()
{
    return &mMoves;
}

void Tank::onPathFound( PieceListManager* path, bool doWakeup )
    if ( action->getFocus() == TANK ) {
    mMoves.reset( path );
    } else {
        mMoves.replaceBack( MOVE );
        mMoves.append( *path->getList() );
    }
    if ( doWakeup ) {
    if ( action->getMoveWhenFound() ) {
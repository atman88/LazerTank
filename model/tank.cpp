#include <iostream>
#include <QEvent>
#include <QCoreApplication>
#include "tank.h"
#include "controller/game.h"
#include "util/imageutils.h"
#include "util/renderutils.h"
#include "util/gameutils.h"

Tank::Tank(QObject* parent) : TankView(parent), mCol(0), mRow(0), mInReset(false)
{
}

void Tank::init( Game* game )
{
    setParent(game);
    AnimationStateAggregator* aggregate = game->getMoveAggregate();
    QObject::connect( &mRotateAnimation,     &QPropertyAnimation::stateChanged, aggregate, &AnimationStateAggregator::onStateChanged );
    QObject::connect( &mHorizontalAnimation, &QPropertyAnimation::stateChanged, aggregate, &AnimationStateAggregator::onStateChanged );
    QObject::connect( &mVerticalAnimation,   &QPropertyAnimation::stateChanged, aggregate, &AnimationStateAggregator::onStateChanged );
    QObject::connect( aggregate, &AnimationStateAggregator::finished, this, &Tank::onAnimationsFinished );

    SpeedController* controller = game->getSpeedController();
    mRotateAnimation.setController( controller );
    mHorizontalAnimation.setController( controller );
    mVerticalAnimation.setController( controller );
    TankView::init( game );
}

void Tank::reset( int col, int row )
{
mInReset = true;
    mCol = col;
    mRow = row;
    mMoves.reset();
    QPoint p( col*24, row*24 );
    TankView::reset( p );
mInReset = false;
}

void Tank::clearMoves()
{
    mMoves.reset();
}

void Tank::move( int direction )
{
    if ( direction < 0 ) {
        followPath();
        return;
    }

    int fromRotation;
    if ( !mMoves.size() ) {
        fromRotation = mViewRotation;
    } else {
        fromRotation = mMoves.getList()->back()->getAngle();
    }
    if ( direction != fromRotation ) {
        if ( !mMoves.size() ) {
            mMoves.append( MOVE, mBoundingRect.left()/24, mBoundingRect.top()/24, direction );
        } else {
            mMoves.replaceBack( MOVE, direction );
        }
        if ( mMoves.size() == 1 ) {
            followPath();
        }
    } else {
        int x, y;
        if ( !mMoves.size() ) {
            x = mBoundingRect.left()/24;
            y = mBoundingRect.top()/24;
        } else {
            Piece* last = mMoves.getList()->back();
            x = last->getCol();
            y = last->getRow();
        }

        Game* game = getGame(this);
        bool hasPush = false;
        if ( game && game->canMoveFrom( TANK, direction, &x, &y, true, &hasPush ) ) {
            mMoves.append( MOVE, x, y, direction, hasPush );

            if ( hasPush ) {
                game->onFuturePush( mMoves.getList()->back() );
            }
            followPath();
        }
    }
}

void Tank::followPath()
{
    Game* game = getGame(this);
    if ( game && !game->getMoveAggregate()->active() ) {
        if ( !mMoves.size() ) {
            emit idled();
            return;
        }

        Piece* move = mMoves.getList()->front();
        int x = move->getCol();
        int y = move->getRow();

        mRotateAnimation.animateBetween( mViewRotation, move->getAngle() );
        mHorizontalAnimation.animateBetween( mBoundingRect.left(), x*24 );
        mVerticalAnimation.animateBetween(   mBoundingRect.top(),  y*24 );

        emit movingInto( x, y, mViewRotation % 360 );
    }
}

int Tank::getRow() const
{
    return mRow;
}

int Tank::getRotation() const
{
    return Shooter::getViewRotation().toInt() % 360;
}

int Tank::getCol() const
{
    return mCol;
}

void Tank::onAnimationsFinished()
{
    if ( !mInReset ) {
        int rotation = getRotation();
        if ( mMoves.size() ) {
            Piece* piece = mMoves.getList()->front();
            if ( piece->getAngle() == rotation
              && piece->getCol() == mCol
              && piece->getRow() == mRow ) {
                mMoves.eraseFront();
            }
            followPath();
        }
    }
}

void Tank::onMoved(int col, int row)
{
    mCol = col;
    mRow = row;
    emit moved( mCol, mRow );
}

PieceListManager* Tank::getMoves()
{
    return &mMoves;
}

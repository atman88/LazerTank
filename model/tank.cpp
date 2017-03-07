#include <iostream>
#include <QEvent>
#include <QCoreApplication>
#include "tank.h"
#include "controller/game.h"
#include "util/imageutils.h"
#include "util/renderutils.h"

Tank::Tank(QObject* parent) : Shooter(parent), mInReset(false)
{
    mRotateAnimation.setTargetObject(this);
    mRotateAnimation.setPropertyName("rotation");
    mHorizontalAnimation.setTargetObject(this);
    mHorizontalAnimation.setPropertyName("x");
    mVerticalAnimation.setTargetObject(this);
    mVerticalAnimation.setPropertyName("y");
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
    Shooter::init( game, QColor(0,255,33) );
}

void Tank::render( const QRect* rect, QPainter* painter )
{
    if ( rect->intersects( mBoundingRect ) ) {
        int x = mBoundingRect.left();
        int y = mBoundingRect.top();
        if ( mRotation != 0 ) {
            renderRotation( x, y, mRotation.toInt(), painter );
        }
        drawPixmap( x, y, TANK, painter );
        if ( !painter->transform().isRotating() ) {
            mPreviousPaintRect = mBoundingRect;
        } else {
            mPreviousPaintRect = painter->transform().mapRect( mBoundingRect );
        }
        painter->resetTransform();
    }

    getShot().render( painter );
}

void Tank::stop()
{
    mRotateAnimation.stop();
    mHorizontalAnimation.stop();
    mVerticalAnimation.stop();
}

void Tank::reset( int boardX, int boardY )
{
    QPoint p( boardX*24, boardY*24 );
    reset( p );
}

void Tank::reset( QPoint& p )
{
    mInReset = true;
        stop();
        Shooter::reset( p );
        mMoves.reset();
        mRotation = 0;
    mInReset = false;
}

void Tank::setX( const QVariant& x )
{
    if ( !mInReset ) {
        int xv = x.toInt();
        if ( xv != mBoundingRect.left() ) {
            QRect dirty( mPreviousPaintRect );
            mBoundingRect.moveLeft( xv );
            dirty |= mBoundingRect;
            emit changed( dirty );

            if ( !(xv % 24) ) {
                emit moved( xv/24, mBoundingRect.top()/24 );
            }
        }
    }
}

void Tank::setY( const QVariant& y )
{
    if ( !mInReset ) {
        int yv = y.toInt();
        if ( yv != mBoundingRect.top() ) {
            QRect dirty( mPreviousPaintRect );
            mBoundingRect.moveTop( yv );
            dirty |= mBoundingRect;
            emit changed( dirty );

            if ( !(yv % 24) ) {
                emit moved( mBoundingRect.left()/24, yv/24 );
            }
        }
    }
}

void Tank::setRotation( const QVariant& angle )
{
    if ( !mInReset ) {
        if ( mRotation != angle ) {
            mRotation = angle;
            emit changed( mPreviousPaintRect );
        }
    }
}

void Tank::move( int direction )
{
    if ( direction < 0 ) {
        followPath();
        return;
    }

    int fromRotation;
    if ( !mMoves.size() ) {
        fromRotation = mRotation.toInt();
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
            x = last->getX();
            y = last->getY();
        }

        Game* game = getGame();
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
    Game* game = getGame();
    if ( game && !game->getMoveAggregate()->active() ) {
        if ( !mMoves.size() ) {
            emit idled();
            return;
        }

        Piece* move = mMoves.getList()->front();
        int curRotation = mRotation.toInt();
        int x = move->getX();
        int y = move->getY();

        mRotateAnimation.animateBetween( curRotation, move->getAngle() );
        mHorizontalAnimation.animateBetween( mBoundingRect.left(), x*24 );
        mVerticalAnimation.animateBetween(   mBoundingRect.top(),  y*24 );

        emit movingInto( x, y, curRotation % 360 );
    }
}

void Tank::onAnimationsFinished()
{
    if ( !mInReset ) {
        int rotation = mRotation.toInt() % 360;
        setRotation( QVariant( rotation ) );
        if ( mMoves.size() ) {
            Piece* piece = mMoves.getList()->front();
            if ( piece->getAngle() == rotation
              && piece->getX() == mBoundingRect.left()/24
              && piece->getY() == mBoundingRect.top()/24 ) {
                mMoves.eraseFront();
            }
            followPath();
        }
    }
}

PieceListManager* Tank::getMoves()
{
    return &mMoves;
}

Game* Tank::getGame()
{
    QObject* p = parent();
    QVariant hv = p->property("GameHandle");
    return hv.value<GameHandle>().game;
}

#include "tankview.h"
#include "model/piece.h"
#include "util/imageutils.h"
#include "util/renderutils.h"

TankView::TankView(QObject *parent) : Shooter(parent)
{
}

void TankView::init(Game *game)
{
    mRotateAnimation.setTargetObject(this);
    mRotateAnimation.setPropertyName("rotation");
    mHorizontalAnimation.setTargetObject(this);
    mHorizontalAnimation.setPropertyName("x");
    mVerticalAnimation.setTargetObject(this);
    mVerticalAnimation.setPropertyName("y");

    Shooter::init( game, QColor(0,255,33) );
}

void TankView::render( const QRect* rect, QPainter* painter )
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

void TankView::stop()
{
    QPropertyAnimation* animations[3] = { &mRotateAnimation, &mHorizontalAnimation, &mVerticalAnimation };
    for( auto it : animations ) {
        if ( it->state() != QPropertyAnimation::Stopped ) {
            it->stop();
        }
    }
}

void TankView::pause()
{
    QPropertyAnimation* animations[3] = { &mRotateAnimation, &mHorizontalAnimation, &mVerticalAnimation };
    for( auto it : animations ) {
        if ( it->state() == QPropertyAnimation::Running ) {
            it->pause();
        }
    }
}

void TankView::resume()
{
    QPropertyAnimation* animations[3] = { &mRotateAnimation, &mHorizontalAnimation, &mVerticalAnimation };
    for( auto it : animations ) {
        if ( it->state() == QPropertyAnimation::Paused ) {
            it->resume();
        }
    }
}

void TankView::reset( QPoint& p )
{
        stop();
        Shooter::reset( p );
        mRotation = 0;
}

void TankView::setX( const QVariant& x )
{
    int xv = x.toInt();
    if ( xv != mBoundingRect.left() ) {
        QRect dirty( mPreviousPaintRect );
        mBoundingRect.moveLeft( xv );
        dirty |= mBoundingRect;
        emit changed( dirty );

        if ( !(xv % 24) ) {
            onMoved( xv/24, mBoundingRect.top()/24 );
        }
    }
}

void TankView::setY( const QVariant& y )
{
    int yv = y.toInt();
    if ( yv != mBoundingRect.top() ) {
        QRect dirty( mPreviousPaintRect );
        mBoundingRect.moveTop( yv );
        dirty |= mBoundingRect;
        emit changed( dirty );

        if ( !(yv % 24) ) {
            onMoved( mBoundingRect.left()/24, yv/24 );
        }
    }
}

void TankView::setRotation( const QVariant& angle )
{
    if ( mRotation != angle ) {
        mRotation = angle;
        emit changed( mPreviousPaintRect );
    }
}


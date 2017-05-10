#include <iostream>
#include "tankview.h"
#include "model/piece.h"
#include "view/boardrenderer.h"
#include "util/imageutils.h"

TankView::TankView(QObject *parent) : Shooter(parent)
{
}

void TankView::init( GameRegistry* registry )
{
    mRotateAnimation.setTargetObject(this);
    mRotateAnimation.setPropertyName("rotation");
    mHorizontalAnimation.setTargetObject(this);
    mHorizontalAnimation.setPropertyName("x");
    mVerticalAnimation.setTargetObject(this);
    mVerticalAnimation.setPropertyName("y");

    Shooter::init( registry, TANK, QColor(0,255,33) );
}

void TankView::render( const QRect* rect, QPainter* painter )
{
    if ( rect->intersects( mBoundingRect ) ) {
        if ( !(mViewRotation % 360) ) {
            mPreviousPaintRect = mBoundingRect;
        } else {
            BoardRenderer::renderRotation( mBoundingRect, mViewRotation, painter );
            mPreviousPaintRect = painter->transform().mapRect( mBoundingRect );
        }
        BoardRenderer::renderPixmap( mBoundingRect, TANK, painter );
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

void TankView::reset( const QPoint& p )
{
    stop();
    Shooter::reset( p );
}

void TankView::setViewX( const QVariant& x )
{
    int xv = x.toInt();
    if ( xv != mBoundingRect.left() ) {
        QRect dirty( mPreviousPaintRect );
        mBoundingRect.moveLeft( xv );
        dirty |= mBoundingRect;
        emit changed( dirty );

        if ( !(xv % 24) && !(mViewRotation % 90)) {
            onMoved( xv/24, mBoundingRect.top()/24, mViewRotation % 360 );
        }
    }
}

void TankView::setViewY( const QVariant& y )
{
    int yv = y.toInt();
    if ( yv != mBoundingRect.top() ) {
        QRect dirty( mPreviousPaintRect );
        mBoundingRect.moveTop( yv );
        dirty |= mBoundingRect;
        emit changed( dirty );

        if ( !(yv % 24) && !(mViewRotation % 90) ) {
            onMoved( mBoundingRect.left()/24, yv/24, mViewRotation % 360 );
        }
    }
}

void TankView::setViewRotation( const QVariant& angle )
{
    if ( mViewRotation != angle ) {
        mViewRotation = angle.toInt();
        emit changed( mPreviousPaintRect );

        if ( !(mBoundingRect.left() % 24) && !(mBoundingRect.top() % 24) && !(mViewRotation % 90) ) {
            onMoved( mBoundingRect.left()/24, mBoundingRect.top()/24, mViewRotation % 360 );
        }
    }
}

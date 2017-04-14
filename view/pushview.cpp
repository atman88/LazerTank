#include <iostream>
#include "push.h"
#include "controller/gameregistry.h"
#include "util/renderutils.h"

PushView::PushView( QObject* parent ) : QObject(parent)
{
    mHorizontalAnimation.setTargetObject(this);
    mHorizontalAnimation.setPropertyName("pieceX");
    mVerticalAnimation.setTargetObject(this);
    mVerticalAnimation.setPropertyName("pieceY");
    QObject::connect( &mHorizontalAnimation, &QVariantAnimation::finished, this, &PushView::onStopped );
    QObject::connect( &mVerticalAnimation,   &QVariantAnimation::finished, this, &PushView::onStopped );
    mHorizontalAnimation.setDuration( 1000 );
    mVerticalAnimation.setDuration( 1000 );

    mType = NONE;
    mBoundingRect.setRect(0,0,24,24);
}

void PushView::init( GameRegistry* registry )
{
    SpeedController& SpeedController = registry->getSpeedController();
    mHorizontalAnimation.setController( &SpeedController );
    mVerticalAnimation.setController( &SpeedController );
}

void PushView::start( Piece& what, int fromX, int fromY, int toX, int toY )
{
    if ( mType != NONE ) {
        return;
    }

    mBoundingRect.moveLeft( fromX );
    mBoundingRect.moveTop( fromY );
    mType = what.getType();
    mPieceAngle = what.getAngle();

    mHorizontalAnimation.animateBetween( fromX, toX );
    mVerticalAnimation.animateBetween( fromY, toY );

    emit stateChanged(QAbstractAnimation::Running, QAbstractAnimation::Stopped);
}

void PushView::render( const QRect* rect, QPainter* painter )
{
    if ( mType != NONE && mBoundingRect.intersects( *rect ) ) {
        renderPiece( mType, mBoundingRect.left(), mBoundingRect.top(), mPieceAngle, painter );
        mRenderedBoundingRect = mBoundingRect;
    }
}

PieceType PushView::getType()
{
    return mType;
}

int PushView::getPieceAngle()
{
    return mPieceAngle;
}

QRect* PushView::getBounds()
{
    return &mBoundingRect;
}

QVariant PushView::getX()
{
    return QVariant( mBoundingRect.left() );
}

QVariant PushView::getY()
{
    return QVariant( mBoundingRect.top() );
}

void PushView::setX( const QVariant& x )
{
    int xv = x.toInt();
    if ( xv != mBoundingRect.left() ) {
        mBoundingRect.moveLeft( xv );
        QRect dirty( mBoundingRect );
        if ( !mRenderedBoundingRect.isNull() ) {
            dirty |= mRenderedBoundingRect;
        }
        emit rectDirty( dirty );
    }
}

void PushView::setY( const QVariant& y )
{
    int yv = y.toInt();
    if ( yv != mBoundingRect.top() ) {
        mBoundingRect.moveTop( yv );
        QRect dirty( mBoundingRect );
        if ( !mRenderedBoundingRect.isNull() ) {
            dirty |= mRenderedBoundingRect;
        }
        emit rectDirty( dirty );
    }
}

void PushView::onStopped()
{
    if ( mType != NONE ) {
        stopping();
        mType = NONE;
        if ( !mRenderedBoundingRect.isNull() ) {
            emit rectDirty( mRenderedBoundingRect );
            mRenderedBoundingRect = QRect();
        }
        emit stateChanged(QAbstractAnimation::Stopped, QAbstractAnimation::Running);
    }
}

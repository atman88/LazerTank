#include <iostream>
#include <QPainter>
#include "push.h"
#include "controller/gameregistry.h"

PushView::PushView( QObject* parent ) : QObject(parent), mType(NONE), mPieceAngle(0)
{
    mHorizontalAnimation.setTargetObject(this);
    mHorizontalAnimation.setPropertyName("pieceX");
    mVerticalAnimation.setTargetObject(this);
    mVerticalAnimation.setPropertyName("pieceY");
    QObject::connect( &mHorizontalAnimation, &QVariantAnimation::finished, this, &PushView::onStopped );
    QObject::connect( &mVerticalAnimation,   &QVariantAnimation::finished, this, &PushView::onStopped );
    mHorizontalAnimation.setDuration( 1000 );
    mVerticalAnimation.setDuration( 1000 );

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
        std::cout << "** PushView::start mType=" << mType << std::endl;
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

void PushView::render( const QRect* rect, BoardRenderer& renderer, QPainter* painter )
{
    if ( mType != NONE && mBoundingRect.intersects( *rect ) ) {
        renderer.renderPiece( mType, mBoundingRect, mPieceAngle, painter );
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

bool PushView::occupies( QPoint point )
{
    if ( mType != NONE ) {
        return mBoundingRect.left() <= point.x() && point.x() <= mBoundingRect.right()
            && mBoundingRect.top()  <= point.y() && point.y() <= mBoundingRect.bottom();
    }
    return false;
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

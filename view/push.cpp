#include <iostream>
#include "push.h"
#include "controller/game.h"
#include "util/renderutils.h"

Push::Push(QObject *parent) : QObject(parent)
{
    mHorizontalAnimation.setTargetObject(this);
    mHorizontalAnimation.setPropertyName("pieceX");
    mVerticalAnimation.setTargetObject(this);
    mVerticalAnimation.setPropertyName("pieceY");
    QObject::connect( &mHorizontalAnimation, &QVariantAnimation::finished, this, &Push::onStopped );
    QObject::connect( &mVerticalAnimation,   &QVariantAnimation::finished, this, &Push::onStopped );
    mHorizontalAnimation.setDuration( 1000 );
    mVerticalAnimation.setDuration( 1000 );

    mType = NONE;
    mBoundingRect.setRect(0,0,24,24);
}

void Push::init( Game* game )
{
    QObject::connect( this, &Push::stateChanged, game->getMoveAggregate(), &AnimationStateAggregator::onStateChanged );
    QObject::connect( this, &Push::stateChanged, game->getShotAggregate(), &AnimationStateAggregator::onStateChanged );

    SpeedController* SpeedController = game->getSpeedController();
    mHorizontalAnimation.setController( SpeedController );
    mVerticalAnimation.setController( SpeedController );
}

void Push::start( Piece& what, int fromX, int fromY, int toX, int toY )
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

void Push::render( const QRect* rect, QPainter* painter )
{
    if ( mType != NONE && mBoundingRect.intersects( *rect ) ) {
        renderPiece( mType, mBoundingRect.left(), mBoundingRect.top(), mPieceAngle, painter );
        mRenderedBoundingRect = mBoundingRect;
    }
}

PieceType Push::getType()
{
    return mType;
}

int Push::getPieceAngle()
{
    return mPieceAngle;
}

QRect* Push::getBounds()
{
    return &mBoundingRect;
}

QVariant Push::getX()
{
    return QVariant( mBoundingRect.left() );
}

QVariant Push::getY()
{
    return QVariant( mBoundingRect.top() );
}

void Push::setX( const QVariant& x )
{
    int xv = x.toInt();
    if ( xv != mBoundingRect.left() ) {
        QRect dirty( mBoundingRect );
        mBoundingRect.moveLeft( xv );
        dirty |= mBoundingRect;
        emit rectDirty( dirty );
    }
}

void Push::setY( const QVariant& y )
{
    int yv = y.toInt();
    if ( yv != mBoundingRect.top() ) {
        QRect dirty( mBoundingRect );
        mBoundingRect.moveTop( yv );
        dirty |= mBoundingRect;
        emit rectDirty( dirty );
    }
}

void Push::onStopped()
{
    if ( mType != NONE ) {
        int x = getX().toInt()/24;
        int y = getY().toInt()/24;
        Game* game = getGame(this);
        if ( game ) {
            game->getBoard()->applyPushResult( mType, x, y, mPieceAngle );
        }
        mType = NONE;
        if ( !mRenderedBoundingRect.isNull() ) {
            emit rectDirty( mRenderedBoundingRect );
            mRenderedBoundingRect = QRect();
        }
        emit stateChanged(QAbstractAnimation::Stopped, QAbstractAnimation::Running);
    }
}

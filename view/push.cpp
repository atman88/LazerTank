#include <iostream>
#include "push.h"
#include "controller/Game.h"

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
    QObject::connect( this, &Push::stateChanged, game->getMoveAggregate(), &AnimationAggregator::onStateChanged );
    QObject::connect( this, &Push::stateChanged, game->getShotAggregate(), &AnimationAggregator::onStateChanged );

    SpeedController* SpeedController = game->getSpeedController();
    mHorizontalAnimation.setController( SpeedController );
    mVerticalAnimation.setController( SpeedController );
}

void Push::start( Piece& what, int fromX, int fromY, int toX, int toY )
{
    if ( mType != NONE ) {
        cout << "Push already started!\n";
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

int Push::getEndX()
{
    if ( mType != NONE ) {
        return mHorizontalAnimation.endValue().toInt();
    }
    return mBoundingRect.left();
}

int Push::getEndY()
{
    if ( mType != NONE ) {
        return mVerticalAnimation.endValue().toInt();
    }
    return mBoundingRect.top();
}

void Push::onStopped()
{
    if ( mType != NONE ) {
        Board* board = getBoard();
        if ( board ) {
            int x = getX().toInt()/24;
            int y = getY().toInt()/24;
            if ( board->tileAt(x,y) != WATER ) {
                board->getPieceManager().insert( mType, x, y, mPieceAngle );
            } else if ( mType == TILE ) {
                board->setTileAt( TILE_SUNK, x, y );
            } else {
                // erase it from the display:
                emit rectDirty( mBoundingRect );
            }
//            cout << "Push finished (" << x << "," << y << ")\n";
        }
        mType = NONE;
        emit stateChanged(QAbstractAnimation::Stopped, QAbstractAnimation::Running);
    }
}

Board* Push::getBoard()
{
    // find the game from the object hierarchy:
    QObject* p = parent();
    QVariant v;
    while( p && !(v = p->property("GameHandle")).isValid() ) {
        p = p->parent();
    }
    Game* game = v.value<GameHandle>().game;
    if ( game ) {
        return game->getBoard();
    }
    return 0;
}

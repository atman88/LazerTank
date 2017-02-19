#include <iostream>
#include "push.h"
#include "controller/Game.h"

Push::Push(QObject *parent) : QObject(parent)
{
    mHorizontalAnimation = new QPropertyAnimation( this, "pieceX" );
    mVerticalAnimation   = new QPropertyAnimation( this, "pieceY" );
    QObject::connect(mHorizontalAnimation, &QVariantAnimation::finished,this,&Push::onStopped);
    QObject::connect(mVerticalAnimation,   &QVariantAnimation::finished,this,&Push::onStopped);
    mHorizontalAnimation->setDuration( 1000 );
    mVerticalAnimation->setDuration( 1000 );

    mType = NONE;
    mBoundingRect.setRect(0,0,24,24);
}

void Push::init( Game* game )
{
    QObject::connect( this, &Push::stateChanged, game->getMoveAggregate(), &AnimationAggregator::onStateChanged );
    QObject::connect( this, &Push::stateChanged, game->getShotAggregate(), &AnimationAggregator::onStateChanged );
}

void Push::start( Piece& what, int fromX, int fromY, int toX, int toY )
{
    if ( mType != NONE ) {
        cout << "Push already started!\n";
        return;
    }

    cout << "Push start " << (fromX/24) << "," << (fromY/24) << "\n";
    mBoundingRect.moveLeft( fromX );
    mBoundingRect.moveTop( fromY );
    mType = what.getType();
    mPieceAngle = what.getAngle();

    if ( toX != fromX ) {
        mHorizontalAnimation->setStartValue( fromX );
        mHorizontalAnimation->setEndValue( toX );
        mHorizontalAnimation->start();
    }

    if ( toY != fromY ) {
        mVerticalAnimation->setStartValue( fromY );
        mVerticalAnimation->setEndValue( toY );
        mVerticalAnimation->start();
    }
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
        emit pieceMoved( dirty );
    }
}

void Push::setY( const QVariant& y )
{
    int yv = y.toInt();
    if ( yv != mBoundingRect.top() ) {
        QRect dirty( mBoundingRect );
        mBoundingRect.moveTop( yv );
        dirty |= mBoundingRect;
        emit pieceMoved( dirty );
    }
}

void Push::onStopped()
{
    if ( mType != NONE ) {
        Board* board = getBoard();
        if ( board ) {
            int x = getX().toInt()/24;
            int y = getY().toInt()/24;
            if ( board->tileAt(x,y) != WATER ) {
                board->addPiece( mType, x, y, mPieceAngle );
            } else if ( mType == TILE ) {
                board->setTileAt( TILE_SUNK, x, y );
            }
            cout << "Push finished (" << x << "," << y << ")\n";
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

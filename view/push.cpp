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

void Push::start( PieceType what, int fromX, int fromY, int toX, int toY )
{
    mBoundingRect.moveLeft( fromX );
    mBoundingRect.moveTop( fromY );
    mType = what;

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
}

PieceType Push::getType()
{
    return mType;
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
            if ( mType == TILE && board->tileAt(x,y) == WATER ) {
                board->setTileAt( TILE_SUNK, x, y );
            } else {
                board->addPiece( mType, x, y );
            }
        }
        mType = NONE;
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

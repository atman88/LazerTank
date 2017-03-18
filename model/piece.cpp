#include <QRect>

#include "piece.h"

Piece::Piece( const Piece *source )
{
    mType  = source->mType;
    mCol   = source->mCol;
    mRow   = source->mRow;
    mAngle = source->mAngle;
}

PieceType Piece::getType() const
{
    return mType;
}

int Piece::getCol() const
{
    return mCol;
}

int Piece::getRow() const
{
    return mRow;
}

int Piece::getAngle() const
{
    return mAngle;
}

void Piece::getBounds( QRect *rect ) const
{
    rect->setRect( mCol*24, mRow*24, 24, 24 );
}

void Piece::setType( PieceType type )
{
    mType = type;
}

void Piece::setAngle(int angle)
{
    mAngle = angle;
}

PusherPiece::PusherPiece(const Piece *source) : SimplePiece(source)
{
    if ( source->hasPush() ) {
        const PusherPiece* pusherSource = dynamic_cast<const PusherPiece*>(source);
        mPushPieceType  = pusherSource->mPushPieceType;
        mPushPieceAngle = pusherSource->mPushPieceAngle;
    } else {
        mPushPieceType  = NONE;
        mPushPieceAngle = 0;
    }
}

PieceType PusherPiece::getPushPieceType() const
{
    return mPushPieceType;
}

int PusherPiece::getPushPieceAngle() const
{
    return mPushPieceAngle;
}

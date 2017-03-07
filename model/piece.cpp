#include <QRect>

#include "piece.h"

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

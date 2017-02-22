#include <QRect>

#include "piece.h"

PieceType Piece::getType() const
{
    return mType;
}

int Piece::getX() const
{
    return mX;
}

int Piece::getY() const
{
    return mY;
}

int Piece::getAngle() const
{
    return mAngle;
}

void Piece::getBounds( QRect& rect ) const
{
    rect.setRect( mX*24, mY*24, 24, 24 );
}

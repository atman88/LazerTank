#ifndef PIECE_H
#define PIECE_H

#include <qmetatype.h>
#include <list>

#define PIECE_MAX_ROWCOUNT 256

typedef enum {
    MOVE,
    TILE
} PieceType;

class Piece
{
public:
    Piece( PieceType type, int x, int y, int angle = 0 ) : mType(type), mX(x), mY(y), mAngle(angle)
    {
    }
    Piece( const Piece& from )
    {
        mType = from.mType;
        mAngle = from.mAngle;
        mX = from.mX;
        mY = from.mY;
    }

    friend bool operator<(const Piece& l, const Piece& r)
    {
        return l.encodedPos() < r.encodedPos();
    }
    PieceType getType() const;
    int getX() const;
    int getY() const;
    int getAngle() const;

    static int encodePos( int x, int y )
    {
        return y * PIECE_MAX_ROWCOUNT + x;
    }

    int encodedPos() const
    {
        return encodePos( mX, mY );
    }

private:
    PieceType mType;
    int mX;
    int mY;
    int mAngle;
};

typedef std::list<Piece> PieceList;
Q_DECLARE_METATYPE(PieceList)

#endif // PIECE_H

#ifndef PIECE_H
#define PIECE_H

#include <qmetatype.h>
#include <list>
#include <set>

#define PIECE_MAX_ROWCOUNT 256

typedef enum {
    NONE,
    TANK,
    MOVE,
    TILE,
    SHOT_STRAIGHT
} PieceType;

class Piece
{
public:
    Piece( PieceType type = NONE, int x = 0, int y = 0, int angle = 0 ) : mType(type), mX(x), mY(y), mAngle(angle)
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
typedef std::set<Piece> PieceSet;
Q_DECLARE_METATYPE(PieceList)
Q_DECLARE_METATYPE(PieceSet)

#endif // PIECE_H

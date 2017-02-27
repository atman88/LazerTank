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
    SHOT_STRAIGHT,
    SHOT_LEFT,
    SHOT_RIGHT,
    SHOT_END,
    TILE_MIRROR,
    CANNON,
    TILE_FUTURE_ERASE,
    TILE_FUTURE_INSERT,
    PieceTypeUpperBound   // must be last
} PieceType;

class Piece
{
public:
    Piece( PieceType type = NONE, int x = 0, int y = 0, int angle = 0 ) : mType(type), mX(x), mY(y), mAngle(angle)
    {
    }

    Piece( Piece& source )
    {
        mType  = source.mType;
        mX     = source.mX;
        mY     = source.mY;
        mAngle = source.mAngle;
    }

    virtual ~Piece()
    {
    }

    static int encodePos( int x, int y )
    {
        return y * PIECE_MAX_ROWCOUNT + x;
    }

    PieceType getType() const;
    int getX() const;
    int getY() const;
    int getAngle() const;
    void getBounds( QRect *rect ) const;
    virtual bool hasPush() const = 0;

    int encodedPos() const
    {
        return encodePos( mX, mY );
    }

    friend bool operator<(const Piece& l, const Piece& r)
    {
        return l.encodedPos() < r.encodedPos();
    }

private:
    PieceType mType;
    int mX;
    int mY;
    int mAngle;
};

class SimplePiece : public Piece
{
public:
    SimplePiece( PieceType type = NONE, int x = 0, int y = 0, int angle = 0 ) : Piece(type,x,y,angle)
    {
    }
    SimplePiece( Piece& source ) : Piece(source)
    {
    }

    bool hasPush() const override
    {
        return false;
    }
};

class PusherPiece : public SimplePiece
{
public:
    PusherPiece( PieceType type = NONE, int x = 0, int y = 0, int angle = 0, bool hasPush = false )
        : SimplePiece(type,x,y,angle), mHasPush(hasPush)
    {
    }

    void setHasPush( bool hasPush )
    {
        mHasPush = hasPush;
    }

    bool hasPush() const override
    {
        return mHasPush;
    }

private:
    bool mHasPush;
};

struct PieceSetComparator {
    bool operator() (const Piece* l, const Piece* r) const
    {
        return l->encodedPos() < r->encodedPos();
    }
};

typedef std::list<Piece*> PieceList;
typedef std::set<Piece*,PieceSetComparator> PieceSet;

#endif // PIECE_H

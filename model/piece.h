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
    TILE_MIRROR,
    CANNON,
    TILE_FUTURE_ERASE,
    TILE_FUTURE_INSERT,
    DAMAGE,
    PieceTypeUpperBound   // must be last
} PieceType;

/**
 * @brief The Piece interface
 * This is an abstract class so that peices can vary in make up.
 */
class Piece
{
public:
    Piece( PieceType type = NONE, int x = 0, int y = 0, int angle = 0 ) : mType(type), mX(x), mY(y), mAngle(angle)
    {
    }

    /**
     * @brief Copy constructor
     */
    Piece( Piece* source )
    {
        mType  = source->mType;
        mX     = source->mX;
        mY     = source->mY;
        mAngle = source->mAngle;
    }

    virtual ~Piece()
    {
    }

    /**
     * @brief Creates a search term used for searching piece sets
     * @param x Column number
     * @param y Row number
     * @return The encoded position value
     */
    static int encodePos( int x, int y )
    {
        return y * PIECE_MAX_ROWCOUNT + x;
    }

    PieceType getType() const;
    void setType( PieceType type );
    int getX() const;
    int getY() const;


    /**
     * @brief Gets the current rotation of this peice
     */
    int getAngle() const;

    /**
     * @brief Changes the current rotation for this piece
     */
    void setAngle( int angle );

    /**
     * @brief Gets the rectangular screen area currently occupied by this piece
     * @param rect The rect to return the result in
     */
    void getBounds( QRect *rect ) const;

    /**
     * @brief Query if this piece is associated with a push operation
     */
    virtual bool hasPush() const = 0;

    /**
     * @brief Create a search term for this piece
     * @param x Column number
     * @param y Row number
     * @return The encoded position value
     */
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

/**
 * @brief A basic piece that has no extended capabilities
 */
class SimplePiece : public Piece
{
public:
    SimplePiece( PieceType type = NONE, int x = 0, int y = 0, int angle = 0 ) : Piece(type,x,y,angle)
    {
    }
    SimplePiece( Piece* source ) : Piece(source)
    {
    }

    bool hasPush() const override
    {
        return false;
    }
};

/**
 * @brief A peice that hints whether it has an associated push
 */
class PusherPiece : public SimplePiece
{
public:
    PusherPiece( PieceType type = NONE, int x = 0, int y = 0, int angle = 0, bool hasPush = false )
        : SimplePiece(type,x,y,angle), mHasPush(hasPush)
    {
    }

    PusherPiece( Piece* source ) : SimplePiece(source), mHasPush(source->hasPush())
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

// comparator used by the stl
struct PieceSetComparator {
    bool operator() (const Piece* l, const Piece* r) const
    {
        return l->encodedPos() < r->encodedPos();
    }
};

// stl container types used for pieces
typedef std::list<Piece*> PieceList;
typedef std::set<Piece*,PieceSetComparator> PieceSet;
typedef std::multiset<Piece*,PieceSetComparator> PieceMultiSet;

#endif // PIECE_H

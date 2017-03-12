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
    Piece( PieceType type = NONE, int col = 0, int row = 0, int angle = 0 ) : mType(type), mCol(col), mRow(row), mAngle(angle)
    {
    }

    /**
     * @brief Copy constructor
     */
    Piece( Piece* source )
    {
        mType  = source->mType;
        mCol     = source->mCol;
        mRow     = source->mRow;
        mAngle = source->mAngle;
    }

    virtual ~Piece()
    {
    }

    /**
     * @brief Creates a search term used for searching piece sets
     * @param col Column number
     * @param row Row number
     * @return The encoded position value
     */
    static int encodePos( int col, int row )
    {
        return row * PIECE_MAX_ROWCOUNT + col;
    }

    PieceType getType() const;
    void setType( PieceType type );
    int getCol() const;
    int getRow() const;

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
     * @brief Retrieve the search term for this piece
     * @return The encoded position value
     */
    int encodedPos() const
    {
        return encodePos( mCol, mRow );
    }

    friend bool operator<(const Piece& l, const Piece& r)
    {
        return l.encodedPos() < r.encodedPos();
    }

private:
    PieceType mType;
    int mCol;
    int mRow;
    int mAngle;
};

/**
 * @brief A basic piece that has no extended capabilities
 */
class SimplePiece : public Piece
{
public:
    SimplePiece( PieceType type = NONE, int col = 0, int row = 0, int angle = 0 ) : Piece(type,col,row,angle)
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
    PusherPiece( PieceType type = NONE, int col = 0, int row = 0, int angle = 0, bool hasPush = false )
        : SimplePiece(type,col,row,angle), mHasPush(hasPush)
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

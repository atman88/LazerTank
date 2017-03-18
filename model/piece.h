#ifndef PIECE_H
#define PIECE_H

#include <qmetatype.h>
#include <list>
#include <set>

#include "pieceview.h"

#define PIECE_MAX_ROWCOUNT 256

/**
 * @brief The Piece interface
 * This is an abstract class so that peices can vary in make up.
 */
class Piece : public PieceView
{
public:
    Piece( PieceType type = NONE, int col = 0, int row = 0, int angle = 0 ) : PieceView(type,col,row,angle)
    {
    }

    /**
     * @brief Copy constructor
     */
    Piece( const Piece* source ) : PieceView(source)
    {
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

    friend class PusherPiece;
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
    SimplePiece( const Piece* source ) : Piece(source)
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
    PusherPiece( PieceType type = NONE, int col = 0, int row = 0, int angle = 0, Piece* pushPiece = 0 )
        : SimplePiece(type,col,row,angle),
          mPushPieceType( pushPiece ? pushPiece->mType  : NONE),
          mPushPieceAngle(pushPiece ? pushPiece->mAngle : 0)
    {
    }

    PusherPiece( const Piece* source );

    bool hasPush() const override
    {
        return mPushPieceType != NONE;
    }

    PieceType getPushPieceType() const;

    int getPushPieceAngle() const;

private:
    PieceType mPushPieceType;
    int mPushPieceAngle;
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

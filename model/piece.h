#ifndef PIECE_H
#define PIECE_H

#include <QObject>
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

    Piece( PieceType type, ModelPoint point = ModelPoint(0,0), int angle = 0 ) : PieceView(type,point,angle)
    {
    }

    Piece( PieceType type, ModelVector vector ) : PieceView(type,vector)
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
     * @brief Retrieve the search term for this piece
     * @return The encoded position value
     */
    int encodedPos() const
    {
        return encodePos( mCol, mRow );
    }

    virtual bool hasPush() const override = 0;
    virtual int getShotCount() const override = 0;
    virtual int getShotPathUID() const override = 0;
    virtual int getPreviousPushedId() const = 0;

    virtual int getPushedId() const override = 0;

    friend bool operator<(const Piece& l, const Piece& r)
    {
        return l.encodedPos() < r.encodedPos();
    }

    friend class MovePiece;
};

/**
 * @brief A basic piece having no extended attributes
 */
class SimplePiece : public Piece
{
public:
    SimplePiece( PieceType type = NONE, int col = 0, int row = 0, int angle = 0 ) : Piece(type,col,row,angle)
    {
    }

    SimplePiece( PieceType type = NONE, ModelPoint point = ModelPoint(0,0), int angle = 0 ) : Piece(type,point,angle)
    {
    }

    SimplePiece( PieceType type, ModelVector vector ) : Piece(type,vector)
    {
    }

    SimplePiece( const Piece* source ) : Piece(source)
    {
    }

    bool hasPush() const override
    {
        return false;
    }

    int getShotCount() const override
    {
        return 0;
    }

    int getShotPathUID() const override
    {
        return 0;
    }

    int getPreviousPushedId() const override
    {
        return 0;
    }

    int getPushedId() const override
    {
        return 0;
    }
};

class PushedPiece : public SimplePiece
{
public:
    PushedPiece( Piece* piece, int pushedId = 0 ) : SimplePiece(piece), mId(pushedId)
    {
    }

    PushedPiece( PieceType type, ModelPoint point, int angle = 0,  int pushedId = 0 )
      : SimplePiece( type, point, angle ), mId(pushedId)
    {
    }

    int getPushedId() const override
    {
        return mId;
    }

private:
    int mId;
};

/**
 * @brief A piece that can hold additional attributes associated with a move
 */
class MovePiece : public SimplePiece
{
public:
    MovePiece( PieceType type = MOVE, int col = 0, int row = 0, int angle = 0, int shotCount = 0, const Piece* pushPiece = 0 )
        : SimplePiece(type,col,row,angle),
          mPushPieceType( pushPiece ? pushPiece->mType  : NONE),
          mPushPieceAngle(pushPiece ? pushPiece->mAngle : 0),
          mShotCount(shotCount), mShotPathUID(0),
          mPreviousPushedId(pushPiece ? pushPiece->getPushedId() : 0 )
    {
    }

    MovePiece( PieceType type, ModelPoint point, int angle = 0,  int shotCount = 0, const Piece* pushPiece = 0 )
      : SimplePiece( type, point, angle ),
        mPushPieceType( pushPiece ? pushPiece->mType  : NONE),
        mPushPieceAngle(pushPiece ? pushPiece->mAngle : 0),
        mShotCount(shotCount), mShotPathUID(0),
        mPreviousPushedId(pushPiece ? pushPiece->getPushedId() : 0 )
    {
    }

    MovePiece( PieceType type, ModelVector vector, int shotCount = 0, const Piece* pushPiece = 0 )
      : SimplePiece( type, vector ),
        mPushPieceType( pushPiece ? pushPiece->mType  : NONE),
        mPushPieceAngle(pushPiece ? pushPiece->mAngle : 0),
        mShotCount(shotCount), mShotPathUID(0),
        mPreviousPushedId(pushPiece ? pushPiece->getPushedId() : 0 )
    {
    }

    MovePiece( const Piece* source );

    bool hasPush() const override;

    PieceType getPushPieceType() const;

    int getPushPieceAngle() const;

    int getShotCount() const override;

    int getPreviousPushedId() const override;

    /**
     * @brief Set the count of future shots for this move point
     */
    bool setShotCount( int count );

    /**
     * @brief Decrement the count of future shots for this move point
     * @return The resultant shot count or -1 if the shot count was already 0
     */
    int decrementShots();

    /**
     * @brief Get the unique identifier which relates this move to its associated shot path
     * @return The unique identifier or 0 if this move does not have an associated shot path
     */
    int getShotPathUID() const override;

    /**
     * @brief Set the unique identifier which relates this move to its associated shot path
     * @param shotPathUID The identifier of the associated FutureShotPath or 0 if none exists
     */
    void setShotPathUID( int shotPathUID );

private:
    PieceType mPushPieceType;
    int mPushPieceAngle;
    int mShotCount;
    int mShotPathUID;
    int mPreviousPushedId;
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

/**
 * @brief The common interface for piece managers
 */
class PieceManager : public QObject
{
    Q_OBJECT

public:
    PieceManager( QObject* parent = 0 ) : QObject(parent)
    {
    }

signals:
    /**
     * @brief Notifies that a new piece was added to the set
     * @param point The square of the new piece
     * @param row The row of the new piece
     */
    void insertedAt( ModelPoint point );

    /**
     * @brief Notifies that a piece was deleted from the set
     * @param point The square of the deleted piece
     */
    void erasedAt( ModelPoint point );

    /**
     * @brief Notifies that a piece was changed
     * @param point The square of the changed piece
     */
    void changedAt( ModelPoint point );
};

#endif // PIECE_H

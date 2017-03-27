#ifndef PIECEVIEW_H
#define PIECEVIEW_H

#include <QPainter>

#include "tile.h"

typedef enum {
    NONE = TileTypeUpperBound, // must be unique from tile values (allows imageutils to combine them)
    TANK,
    MOVE,
    MOVE_HIGHLIGHT,
    TILE,
    TILE_MIRROR,
    CANNON,
    TILE_FUTURE_ERASE,
    TILE_FUTURE_INSERT,
    DAMAGE,
    PieceTypeUpperBound   // must be last
} PieceType;


class PieceView
{
public:
    PieceView( PieceType type = NONE, int col = 0, int row = 0, int angle = 0 )
      : mType(type), mCol(col), mRow(row), mAngle(angle)
    {
    }

    /**
     * @brief Copy constructor
     */
    PieceView( const PieceView* source );

    virtual ~PieceView()
    {
    }

    bool render( const QRect *dirty, QPainter* painter );

    /**
     * @brief Gets the rectangular screen area currently occupied by this piece
     * @param rect The rect to return the result in
     */
    void getBounds( QRect *rect ) const;

    /**
     * @brief Getters & setters
     */
    PieceType getType() const;
    void setType( PieceType type );
    int getCol() const;
    int getRow() const;

    /**
     * @brief Gets the current rotation of this piece
     */
    int getAngle() const;

    /**
     * @brief Changes the current rotation for this piece
     */
    void setAngle( int angle );

    /**
     * @brief Query whether this piece contains an associated push
     * @return true if this piece causes a push
     */
    virtual bool hasPush() const = 0;

    /**
     * @brief Get the number of shots associated with this piece
     * @return The number of associated shots
     */
    virtual int getShotCount() const = 0;

    /**
     * @brief Get the unique identifier which relates this move to its associated shot path
     * @return The unique identifier or 0 if this move does not have an associated shot path
     */
    virtual int getShotPathUID() const = 0;

protected:
    PieceType mType;
    int mCol;
    int mRow;
    int mAngle;
};

#endif // PIECEVIEW_H

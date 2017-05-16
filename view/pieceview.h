#ifndef PIECEVIEW_H
#define PIECEVIEW_H

#include <QPainter>

#include "tile.h"
#include "model/modelpoint.h"

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

class BoardRenderer;

class PieceView : public ModelVector
{
public:
    PieceView( PieceType type = NONE, int col = 0, int row = 0, int angle = 0 ) : ModelVector(col,row,angle), mType(type)
    {
    }

    PieceView( PieceType type, ModelPoint& point, int angle = 0 ) : ModelVector(point,angle), mType(type)
    {
    }

    PieceView( PieceType type, ModelVector& vector ) : ModelVector(vector), mType(type)
    {
    }

    /**
     * @brief Copy constructor
     */
    PieceView( const PieceView* source );

    virtual ~PieceView()
    {
    }

    /**
     * @brief Paint the piece
     * @param dirty The area that requires painting
     * @param renderer The renderer to use
     * @param painter The painter
     * @return true if painted or false if outside of the dirty area
     */
    bool render( const QRect* dirty, const BoardRenderer& renderer, QPainter* painter );

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
};

#endif // PIECEVIEW_H

#ifndef PIECEVIEW_H
#define PIECEVIEW_H

#include <QPainter>

typedef enum {
    NONE,
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
    PieceView( PieceType type = NONE, int col = 0, int row = 0, int angle = 0 ) : mType(type), mCol(col), mRow(row),
      mAngle(angle), mShotCount(0)
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
     * @brief Get the number of shots for this move
     * @return The number of shots
     */
    int getShotCount() const;

    /**
     * @brief Increment the count of future shots for this move point
     * @return The number of shots
     */
    int incrementShots();

    /**
     * @brief Decrement the count of future shots for this move point
     * @return The resultant shot count or -1 if the shot count was already 0
     */
    int decrementShots();

protected:
    PieceType mType;
    int mCol;
    int mRow;
    int mAngle;
    int mShotCount;
};

#endif // PIECEVIEW_H

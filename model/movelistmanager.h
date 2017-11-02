#ifndef MOVELISTMANAGER_H
#define MOVELISTMANAGER_H

#include "piecelistmanager.h"


class MoveListManager : public PieceListManager
{
public:
    MoveListManager( PieceType initialFocus = TANK, QObject* parent = 0 );

    /**
     * @brief Set the initial focus to be used by this list
     */
    void setInitialFocus( PieceType initialFocus );

    /**
     * @brief Adds a new move piece to the end of this list from the given values
     */
    MovePiece* append( PieceType type, ModelPoint point, int angle, int shotCount, const Piece* pushPiece = 0 );
    MovePiece* append( PieceType type, ModelVector vector, int shotCount, const Piece* pushPiece = 0 );

    /**
     * @brief Set the number of future shots on the last element
     * @return the updated element or 0 if not updated
     */
    MovePiece* setShotCountBack( int count );

    /**
     * @brief Query the starting vector that this list is to use (I.e. Hints what to append when empty)
     * @return
     */
    ModelVector getInitialVector();

private:
    PieceType mInitialFocus;
};

#endif // MOVELISTMANAGER_H

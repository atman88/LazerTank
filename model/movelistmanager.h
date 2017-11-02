#ifndef MOVELISTMANAGER_H
#define MOVELISTMANAGER_H

#include "piecelistmanager.h"


class MoveListManager : public PieceListManager
{
public:
    MoveListManager(  QObject *parent = 0 );

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

};

#endif // MOVELISTMANAGER_H

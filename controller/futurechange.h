#ifndef FUTURECHANGE_H
#define FUTURECHANGE_H

#include "board.h"

/**
  * types of future changes
  */
typedef enum {
    NO_CHANGE,
    TILE_CHANGE,  // The type of tile deleted, or if WOOD then indicates a decay from WOOD to WOOD_DAMAGED
    PIECE_DESTROYED,
    PIECE_PUSHED
} FutureChangeType;

/**
 * @brief A record that describes a push one or more times
 */
typedef struct {
public:
    PieceType pieceType;
    int pieceAngle;
    int direction;
    int count;
} FutureShotMultiPush;

/**
 * @brief A record that describes a single board change (i.e. transaction)
 **/
typedef struct {
    FutureChangeType changeType;
    ModelPoint endCoord; // the last affected square
    union {
        /**
         * @brief TILE_CHANGE info
         */
        TileType tileType;

        /**
         * @brief PIECE_DESTROYED info
         */
        PieceType pieceType;

        /**
         * @brief PIECE_PUSHED info
         */
        FutureShotMultiPush multiPush;
    } u;
} FutureChange;

#endif // FUTURECHANGE_H

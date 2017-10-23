#ifndef FUTURECHANGE_H
#define FUTURECHANGE_H

#include "model/modelpoint.h"
#include "tile.h"

/**
  * types of future changes
  */
typedef enum {
    NO_CHANGE,
    TILE_CHANGE,  // The type of tile deleted, or if WOOD then indicates a decay from WOOD to WOOD_DAMAGED
    PIECE_ERASED,
    PIECE_PUSHED
} FutureChangeType;

/**
 * @brief A record that describes a piece removal
 */
typedef struct {
    PieceType pieceType;
    int pieceAngle;
} FutureChangePieceErase;

/**
 * @brief A record that describes a push one or more times
 */
typedef struct {
    PieceType pieceType;
    int pieceAngle;
    int direction;
    int count;
    int previousPushedId;
} FutureChangeMultiPush;

/**
 * @brief A record that describes a single board change (i.e. transaction)
 **/
typedef struct {
    FutureChangeType changeType;
    ModelPoint point; // the impacted square
    union {
        /**
         * @brief TILE_CHANGE info
         */
        TileType tileType;

        /**
         * @brief PIECE_ERASED info
         */
        FutureChangePieceErase erase;

        /**
         * @brief PIECE_PUSHED info
         */
        FutureChangeMultiPush multiPush;
    } u;
} FutureChange;

#endif // FUTURECHANGE_H

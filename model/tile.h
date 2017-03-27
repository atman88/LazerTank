#ifndef TILE_H
#define TILE_H

// Things that board squares (tiles) can be:
typedef enum {
    DIRT,
    TILE_SUNK,
    STONE,
    WATER,
    FLAG,
    EMPTY,
    STONE_MIRROR,
    STONE_MIRROR__90,
    STONE_MIRROR_180,
    STONE_MIRROR_270,
    STONE_SLIT,
    STONE_SLIT_90,
    WOOD,
    WOOD_DAMAGED,
    TileTypeUpperBound // must be last
} TileType;

#endif // TILE_H

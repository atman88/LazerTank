#ifndef BOARD_H
#define BOARD_H

#include <string>
using namespace std;

#define BOARD_MAX_WIDTH 100
#define BOARD_MAX_HEIGHT 100

typedef enum {
    DIRT,
    STONE,
    WATER
} BoardTileId;

class Board {
public:
    Board( const string& sourceFileName );

public:
    int getWidth();
    int getHeight();
    BoardTileId tileAt( int x, int y );

    void load( const string& fileName );

    int mInitialTankX;
    int mInitialTankY;

private:
    int mHeight;
    int mWidth;

    BoardTileId mTiles[BOARD_MAX_WIDTH*BOARD_MAX_HEIGHT];
};

#endif // BOARD_H

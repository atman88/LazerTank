#ifndef BOARD_H
#define BOARD_H

#include <QObject>
#include <string>

#include "model/piece.h"

#define BOARD_MAX_WIDTH  PIECE_MAX_ROWCOUNT
#define BOARD_MAX_HEIGHT PIECE_MAX_ROWCOUNT

using namespace std;

typedef enum {
    DIRT,
    STONE,
    WATER
} BoardTileId;

class Board : public QObject
{
    Q_OBJECT

public:
    Board( const string& sourceFileName, QObject* parent = 0 );

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

    PieceSet mPieces;
};

#endif // BOARD_H

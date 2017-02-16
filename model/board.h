#ifndef BOARD_H
#define BOARD_H

#include <QObject>

#include "model/piece.h"

#define BOARD_MAX_LEVEL 4

#define BOARD_MAX_WIDTH  PIECE_MAX_ROWCOUNT
#define BOARD_MAX_HEIGHT PIECE_MAX_ROWCOUNT

using namespace std;

typedef enum {
    DIRT,
    TILE_SUNK,
    STONE,
    WATER,
    FLAG
} BoardTileId;

class Board : public QObject
{
    Q_OBJECT

public:
    Board( QObject* parent = 0 );

    int getLevel();
    int getWidth();
    int getHeight();
    BoardTileId tileAt( int x, int y );
    void setTileAt( BoardTileId, int x, int y );
    PieceType pieceAt( int x, int y );
    void erasePieceAt( int x , int y );
    void addPiece( PieceType type, int x, int y );

    bool load( int level );
    bool load( QString& fileName );

    int mInitialTankX;
    int mInitialTankY;

signals:
    void boardLoaded();
    void tileChanged(int x, int y);

private:
    int mLevel;
    int mHeight;
    int mWidth;

    BoardTileId mTiles[BOARD_MAX_WIDTH*BOARD_MAX_HEIGHT];

    PieceSet mPieces;
};

#endif // BOARD_H

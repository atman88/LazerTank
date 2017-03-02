#ifndef BOARD_H
#define BOARD_H

#include <QObject>

#include "model/piecesetmanager.h"

#define BOARD_MAX_LEVEL 20

#define BOARD_MAX_WIDTH  PIECE_MAX_ROWCOUNT
#define BOARD_MAX_HEIGHT PIECE_MAX_ROWCOUNT

using namespace std;

typedef enum {
    DIRT = PieceTypeUpperBound,
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
    TileTypeUpperBound
} TileType;

class Board : public QObject
{
    Q_OBJECT

public:
    Board( QObject* parent = 0 );

    int getLevel();
    int getWidth();
    int getHeight();
    TileType tileAt( int x, int y ) const;
    void setTileAt( TileType, int x, int y );
    PieceSetManager& getPieceManager();
    bool canSightThru( int x, int y );
    bool load( int level );
    bool load( QString& fileName );

    int mInitialTankX;
    int mInitialTankY;

signals:
    void boardLoaded();
    void tileChangedAt(int x, int y) const;

private:
    void initPiece( PieceType type, int x, int y, int angle = 0 );
    int mLevel;
    int mWidth;
    int mHeight;

    unsigned char mTiles[BOARD_MAX_WIDTH*BOARD_MAX_HEIGHT];

    PieceSetManager mPieceManager;
};

#endif // BOARD_H

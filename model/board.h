#ifndef BOARD_H
#define BOARD_H

#include <QObject>

#include "model/piecesetmanager.h"

// The last /maps/level%1.txt file we wish to reach. Increase this as new levels are added.
#define BOARD_MAX_LEVEL 34

// The largest board dimensions we care to support
#define BOARD_MAX_WIDTH  PIECE_MAX_ROWCOUNT
#define BOARD_MAX_HEIGHT PIECE_MAX_ROWCOUNT

// Things that board squares (tiles) can be:
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

/**
 * @brief The Board class
 * A board contains a 2D map of tiles and an associated list of pieces
 */
class Board : public QObject
{
    Q_OBJECT

public:
    Board( QObject* parent = 0 );

    /**
     * @brief Get the current level number. A level number corresponds to a /map/level%1.txt file.
     * @return The level number, or 0 if a level has not been explicitly loaded.
     */
    int getLevel();

    /**
     * @brief Get the total number of columns for this board
     * @return The width in number of squares
     */
    int getWidth();

    /**
     * @brief Get the total number of rows for this board
     * @return The height in number of squares
     */
    int getHeight();

    /**
     * @brief Query what the given square is
     * @param x The column for the square of interest
     * @param y The row for the square of interest
     * @return The type of square
     */
    TileType tileAt( int x, int y ) const;

    /**
     * @brief Change the type for a given square
     * @param x The column for the square to change
     * @param y The row for the square to change
     */
    void setTileAt( TileType, int x, int y );

    /**
     * @brief Get managed access to the peices on this board
     * @return The manager for the pieces on this board
     */
    PieceSetManager* getPieceManager();

    /**
     * @brief Update the board as a result of the given push
     * @param mType The type of piece being pushed
     * @param x The columnn that the piece is being pushed to
     * @param y The row that the piece is being pushed to
     * @param pieceAngle The piece's current rotation
     * @return true if the push resulted in a change to the board
     */
    bool applyPushResult( PieceType mType, int x, int y , int pieceAngle );

    /**
     * @brief load a level
     * @param level A level number between 1 and BOARD_MAX_LEVEL
     * @return true if successful
     */
    bool load( int level );

    /**
     * @brief load the given file
     * @param fileName
     * @return true if successful
     */
    bool load( QString& fileName );

    /**
     * @brief load a copy of the given board
     * @param source The board to copy
     */
    void load( const Board* source );

    /**
     * @brief Get the column that the flag is on for this board
     * @return The column of the flag or -1 if the board does not contain a flag (malformed).
     */
    int getFlagX() const;

    /**
     * @brief Get the row that the flag is on for this board
     * @return The row of the flag or -1 if the board does not contain a flag (malformed).
     */
    int getFlagY() const;

    /**
     * @brief Get the column that the tank was loaded at for this board
     * @return The tank way point column. Column 0 is returned when not specified by the previous load.
     */
    int getTankStartCol() const;

    /**
     * @brief Get the row that the tank was loaded at for this board
     * @return The tank way point column. Column 0 is returned when not specified by the previous load.
     */
    int getTankStartRow() const;

signals:
    /**
     * @brief Signals that the contents of this board has been replaced (via one of its load methods)
     */
    void boardLoaded();

    /**
     * @brief Signals that the given square has changed
     * @param x The square's column
     * @param y The square's row
     */
    void tileChangedAt(int x, int y) const;

private:
    void initPiece( PieceType type, int x, int y, int angle = 0 );
    int mLevel;
    int mWidth;
    int mHeight;
    int mFlagX;
    int mFlagY;
    int mTankWayPointX;
    int mTankWayPointXY;

    unsigned char mTiles[BOARD_MAX_WIDTH*BOARD_MAX_HEIGHT];

    PieceSetManager mPieceManager;
};

#endif // BOARD_H

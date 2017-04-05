#ifndef BOARD_H
#define BOARD_H

#include <QObject>
#include <QTextStream>

#include "tile.h"
#include "model/piecesetmanager.h"
#include "controller/futurechange.h"

// The last /maps/level%1.txt file we wish to reach. Increase this as new levels are added.
#define BOARD_MAX_LEVEL 45

// The largest board dimensions we care to support
#define BOARD_MAX_WIDTH  PIECE_MAX_ROWCOUNT
#define BOARD_MAX_HEIGHT PIECE_MAX_ROWCOUNT

/**
 * @brief Helper method to determine the neighbor square for the given direction
 * @param angle The direction. Legal values are 0, 90, 180, 270.
 * @param point Inputs the starting position. Returns the resultant position
 * @return true if the angle is legal
 */
bool getAdjacentPosition( int angle, ModelPoint *point );

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
     * @param col The column for the square of interest
     * @param row The row for the square of interest
     * @return The type of square
     */
    TileType tileAt( int col, int row ) const;

    /**
     * @brief Change the type for a given square
     * @param col The column for the square to change
     * @param row The row for the square to change
     */
    void setTileAt( TileType, int col, int row );

    /**
     * @brief Get managed access to the pieces on this board
     * @return The manager for the pieces on this board
     */
    PieceSetManager* getPieceManager();

    /**
     * @brief Update the board as a result of the given push
     * @param mType The type of piece being pushed
     * @param col The columnn that the piece is being pushed to
     * @param row The row that the piece is being pushed to
     * @param pieceAngle The piece's current rotation
     * @return true if the push resulted in a change to the board
     */
    bool applyPushResult( PieceType mType, int col, int row, int pieceAngle );

    /**
     * @brief load a level
     * @param level A level number between 1 and BOARD_MAX_LEVEL
     * @return true if successful
     */
    bool load( int level );

    /**
     * @brief reload the current board
     * @return true if successful
     */
    bool reload();

    /**
     * @brief load the given file
     * @param fileName
     * @param level optional level number to associate with the instance or -1 if the file does not correspond to a level
     * @return true if successful
     */
    bool load( QString& fileName, int level = -1 );

    /**
     * @brief load from the given stream
     * @param level optional level number to associate with the instance or -1 if the stream does not correspond to a level
     */
    void load( QTextStream& stream, int level = -1 );

    /**
     * @brief load a copy of the given board
     * @param source The board to copy
     */
    void load( const Board* source );

    /**
     * @brief Get the location of the flag
     */
    ModelPoint getFlagPoint() const;

    /**
     * @brief Get square that the tank was loaded at for this board
     * @return The tank way point. The point 0,0 is returned when not specified in the previously loaded board data.
     */
    ModelPoint getTankStartPoint() const;

    /**
     * @brief roll back future board changes for this shot path
     */
    void undoChanges( std::vector<FutureChange> changes );

signals:
    /**
     * @brief Signals that the contents of this board has been replaced (via one of its load methods)
     */
    void boardLoaded();

    /**
     * @brief Signals that the given square has changed
     * @param col The square's column
     * @param row The square's row
     */
    void tileChangedAt( int col, int row ) const;

private:
    void initPiece( PieceType type, int col, int row, int angle = 0 );
    int mLevel;
    int mWidth;
    int mHeight;
    ModelPoint mFlagPoint;
    ModelPoint mTankWayPoint;

    unsigned char mTiles[BOARD_MAX_WIDTH*BOARD_MAX_HEIGHT];

    PieceSetManager mPieceManager;

    QTextStream* mStream;
};

#endif // BOARD_H

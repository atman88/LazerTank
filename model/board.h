#ifndef BOARD_H
#define BOARD_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QTextStream)

#include "tile.h"
#include "model/piecesetmanager.h"
#include "controller/futurechange.h"

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
    ~Board();

    /**
     * @brief Get the current level number. A level number corresponds to a /map/level%1.txt file.
     * @return The level number, or 0 if a level has not been explicitly loaded.
     */
    int getLevel();

    /**
     * @brief Get the maximum point on this board
     */
    const ModelPoint& getLowerRight() const;

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
     * @param point The square of interest
     * @return The type of square
     */
    TileType tileAt( ModelPoint point ) const;

    /**
     * @brief Change the type for a given square
     * @param point The square to change
     */
    void setTileAt( TileType, ModelPoint point );

    /**
     * @brief Get managed access to the pieces on this board
     * @return The manager for the pieces on this board
     */
    PieceSetManager& getPieceManager();

    /**
     * @brief Update the board as a result of the given push
     * @param mType The type of piece being pushed
     * @param point The square that the piece is being pushed to
     * @param pieceAngle The piece's current rotation
     */
    void applyPushResult( PieceType mType, ModelPoint point, int pieceAngle );

    void revertPush( MovePiece* pusher );

    /**
     * @brief load a level
     * @param level A level number between 1 and BOARD_MAX_LEVEL
     */
    void load( int level );

    /**
     * @brief reload the current board
     */
    void reload();

    /**
     * @brief load the given file
     * @param fileName
     * @param level optional level number to associate with the instance or -1 if the file does not correspond to a level
     * @return true if successful
     */
    bool load( const QString& fileName, int level = -1 );

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
    const ModelVector& getTankStartVector() const;

    /**
     * @brief roll back future board changes for this shot path
     * @param undoShotCount The number of shots to roll back
     * @param changes The changes to roll back
     */
    void undoChanges( int undoShotCount, std::vector<FutureChange> changes );

    int getLastPushId() const;

signals:
    /**
     * @brief Notifies that the board is being loaded in the background
     */
    void boardLoading( int level );

    /**
     * @brief Signals that the contents of this board has been replaced (via one of its load methods)
     */
    void boardLoaded( int level );

    /**
     * @brief Signals that the given square has changed
     * @param point The square's model coordinates
     */
    void tileChangedAt( ModelPoint point ) const;

private:
    void initPiece( PieceType type, int col, int row, int angle = 0 );
    int mLevel;
    ModelPoint mLowerRight;
    ModelPoint mFlagPoint;
    ModelVector mTankWayPoint;
    int mLastPushId;

    unsigned char mTiles[BOARD_MAX_WIDTH*BOARD_MAX_HEIGHT];
    PieceSetManager mPieceManager;

    QTextStream* mStream;
};

#endif // BOARD_H

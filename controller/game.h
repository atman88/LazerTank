#ifndef GAME_H
#define GAME_H

#include <QObject>

class BoardWindow;
class ShotModel;
class PathFinderController;
class Shooter;

#include "gameregistry.h"
#include "model/boarddelta.h"
#include "model/board.h"

/**
 * @brief The Game class responsible for controlling/implementing overall game logic
 */
class Game : public QObject
{
    Q_OBJECT

public:
    Game();
    ~Game() override = default;
    void init( GameRegistry* registry );

    /**
     * @brief Get the current board
     */
    Board* getBoard( bool futuristic = false );

    /**
     * @brief Query whether the current board is the master game board
     * @param board
     * @return true if the board is the master otherwise false
     */
    bool isMasterBoard( Board* board );

    /**
     * @brief Determine whether the given single move is legal
     * @param what The type of peice being moved
     * @param angle The direction to move in. Must be one of 0, 90, 180 or 270
     * @param point The Piece's originating position as input. Returns the resultant position
     * @param futuristic If true, all outstanding moves are considered, otherwise only the current board state is considered
     * @param pushPiece if non-null, returns a reference to any piece that this move would push
     * @return true if the move is allowed, otherwise false
     */
    bool canMoveFrom( PieceType what, int angle, ModelPoint *point, bool futuristic, Piece **pushPiece = nullptr );

    /**
     * @brief Determines the outcome of a laser shot through the given square
     * @param point The square of the lazer end point
     * @param angle Inputs the laser direction as it enters the square; outputs the direction the laser exits the square
     * @param change Apply to the future board if non-zero, otherwise uses the master board. Returns future hit details if non-zero.
     * @param apply If false then game state is not affected, otherse then resultant effects are applied to the game.
     * @param source The producer of laser beam or 0
     * @param hitPoint If non-zero, input as the square's center view coordinate, outputs the hit coordinate (if any)
     * @return true if the the shot is continuing to advance past the square or false if the shot hit something
     */
    bool canShootThru( const ModelPoint& point, int *angle, FutureChange *change = nullptr, bool apply = false, Shooter* source = nullptr,
                       QPoint *hitPoint = nullptr );

    /**
     * @brief Determines whether the given piece can move to the given square. Pending moves are not considered.
     * @param what The type of piece
     * @param point The square to consider
     * @param fromAngle The entry direction
     * @param pushPiece if non-null, returns a reference to any piece that this placement would result in pushing
     * @param futuristic If true, all outstanding moves are considered, otherwise only the current board state is considered
     * @return true if the placement is a legal move
     */
    bool canPlaceAt(PieceType what, const ModelPoint& point, int fromAngle, bool futuristic = false, Piece **pushPiece = nullptr );

    /**
     * @brief Records a push of piece that will be pushed as a result of some future change
     * @param pushPiece The piece that is to be pushed
     * @param direction the direction of the push
     */
    void onFuturePush( Piece* pushPiece, int direction );

    /**
     * @brief Reverts a future push (recorded by onFuturePush)
     * @param pusher Identifies the push to revert based on its originating square and direction (angle)
     */
    void undoFuturePush( MovePiece* pusher );

    /**
     * @brief test whether a piece allows being pushed from the given approach angle
     * @param piece
     * @param fromAngle
     * @return true if the angle is allowed
     */
    static bool canPushPiece( const Piece* piece, int fromAngle );

    /**
     * @brief Obtain the set of pieces representing differences between the current board and
     * what the board will be as a result of applying outstanding moves
     * @return set of future pieces or 0 if future tracking is not active
     */
    const PieceSet* getDeltaPieces();

    void loadMasterBoard( int level );

    /**
     * @brief Query whether the board is fully loaded
     * @return true if the board is loaded or false if the board is in the process of being loaded or has yet to be loaded
     */
    bool isBoardLoaded();

public slots:
    /**
     * @brief Receives notification that the tank is about to move toward the identified square
     * @param point The target square
     * @param fromAngle The direction the tank will move in
     */
    void onTankPushingInto( const ModelPoint& point, int fromAngle );

    /**
     * @brief Notifies the game that the board is being loaded
     */
    void onBoardLoading( int level );

    /**
     * @brief Receives notification that the board's map changed
     */
    void onPoolLoaded( int level );

    /**
     * @brief Receives notification that the master board's map changed
     */
    void onBoardLoaded( int level );

    /**
     * @brief Receives notification that a tile on the board changed.
     * @param point The square where the change occurred
     */
    void onBoardTileChanged( const ModelPoint& point );

    /**
     * @brief Recieves notification that a push has completed
     * @param type The type of piece being pushed
     * @param point The square pushed into
     * @param pieceAngle The orientation of the piece
     */
    void onPushed(PieceType type, const ModelPoint& point, int pieceAngle );

    /**
     * @brief Internal slot for disabling future deltas
     */
    void endMoveDeltaTracking();

    /**
     * @brief Notifies that the current move has completed
     */
    void onMoveAggregatorFinished();

    /**
     * @brief Looks for a cannon that can shoot the tank. Fires the first one it finds.
     */
    void sightCannons();

    /**
     * @brief Handler for when the tank is destroyed
     */
    void onTankKilled();

    /**
     * @brief Restart the current level
     * @param replay If true, restarts and automates an instant replay
     */
    void restartLevel( bool replay = false );

    /**
     * @brief Automate an instant replay of the current level
     */
    void replayLevel();

signals:
    /**
     * @brief Emitted after the board is loaded and interested game objects have updated
     */
    void boardLoaded();

private:
    /**
     * @brief Determines whether the given single move is legal
     * @param what The type of peice being moved
     * @param angle The direction to move in. Must be one of 0, 90, 180 or 270.
     * @param point The Piece's originating square. Returns the resultant square.
     * @param board The board to consider
     * @param pushResult Set to true if a piece would get pushed by the move
     * @return true if allowed, otherwise false
     */
    bool canMoveFrom( PieceType what, int angle, ModelPoint *point, Board* board, Piece **pushPiece = nullptr );

    /**
     * @brief Determines whether the given piece can enter the given square.
     * @param what The type of peice.
     * @param point The square to consider
     * @param fromAngle The entry direction or -1 to simply test the square's vacancy
     * @param board The board to consider
     * @param pushPiece if non-null, returns a reference to any piece that that this placement would result in pushing
     * @return true if the entry is legal
     */
    bool canPlaceAt( PieceType what, ModelPoint point, int fromAngle, Board* board, Piece **pushPiece = nullptr );

    /**
     * @brief determine whether the given square is suitable for a cannon to "see through"
     * @param board The board to use for the test
     * @param point The square of interest
     * @return true if sight is not obstructed
     */
    bool canCannonSightThru( Board* board, ModelPoint point );

    Board mBoard;
    int mDesiredLevel;

    Board mFutureBoard;

protected:
    BoardDelta mFutureDelta;
};

#endif // GAME_H

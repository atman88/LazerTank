#ifndef GAME_H
#define GAME_H

#include <QObject>

struct GameHandle;
class BoardWindow;
class ShotModel;
class PathFinderController;

#include "animationstateaggregator.h"
#include "pathfindercontroller.h"
#include "model/boarddelta.h"
#include "model/board.h"
#include "model/tank.h"
#include "view/push.h"
#include "view/shooter.h"
#include "util/gameutils.h"

/**
 * @brief The Game class responsible for controlling/implementing overall game logic
 */
class Game : public QObject
{
    Q_OBJECT

public:
    Game();
    GameHandle getHandle();
    void init( BoardWindow* window );

    /**
     * @brief Get the current board
     */
    Board* getBoard();

    /**
     * @brief Get this game's main window
     * @return The associated window, or 0 if not initialized
     */
    BoardWindow* getWindow() const;

    /**
     * @brief Access the game's tank object
     */
    Tank* getTank();

    /**
     * Container for the piece currently being pushed by the tank
     * It's type is NONE when the tank isn't pushing a piece
     */
    Push& getTankPush();

    /**
     * Container for the piece currently being pushed by a shot
     * It's type is NONE when no piece is being shot
     */
    Push& getShotPush();

    /**
     * @brief the current laser shot being fired
     * Only one cannon is fired at a time. This is the single lazer path shot from any single selected cannon
     */
    ShotModel& getCannonShot();

    /**
     * @brief Access to the controller that manages the game speed used by animations
     */
    SpeedController* getSpeedController();

    /**
     * @brief Holds the aggregate state of the current tank move including any push the move is doing
     */
    AnimationStateAggregator* getMoveAggregate();

    /**
     * @brief Holds the aggregate state of active shots including any pushes the shot are doing
     * @return
     */
    AnimationStateAggregator* getShotAggregate();

    /**
     * @brief Determine whether the given single move is legal
     * @param what The type of peice being moved
     * @param angle The direction to move in. Must be one of 0, 90, 180 or 270
     * @param col The Piece's originating column as input. Returns the resultant column
     * @param row The Piece's originating row as input. Returns the resultant row
     * @param futuristic If true, all outstanding moves are considered, otherwise only the current board state is considered
     * @param pushResult If nonzero, this returns a boolean indicating whether a piece would be pushed by the move
     * @return true if the move is allowed, otherwise false
     */
    bool canMoveFrom(PieceType what, int angle, int *col, int *row, bool futuristic, bool* pushResult = 0 );

    /**
     * @brief Determines the outcome of a laser shot through the given square
     * @param col The column of the lazer end point
     * @param row The row of the lazer end point
     * @param angle Inputs the laser direction as it enters the square; outputs the direction the laser exits the square
     * @param source The laser beam being produced
     * @param hit Input as the square's center view coordinate. Returns the hit coordinate when something hit
     * @return true if the the shot is continuing to advance past the square or false if the shot hit something
     */
    bool canShootThru( int col, int row, int *angle, Shooter* source, QPoint *hitPoint );

    /**
     * @brief Determines whether the given piece can move to the given square. Pending moves are not considered.
     * @param what The type of piece
     * @param col The column of the square to consider
     * @param row The row of the square to consider
     * @param fromAngle The entry direction
     * @param pushResult true if the entry would result in pushing a piece
     * @return true if the placement is a legal move
     */
    bool canPlaceAtNonFuturistic(PieceType what, int col, int row, int fromAngle, bool *pushResult = 0);

    /**
     * @brief Records a push of piece that will be pushed as a result of some future change
     * @param col The starting column of the future piece
     * @param row The starting row of the future piece
     * @param direction the direction of the push
     */
    void onFuturePush( int col , int row, int direction );

    /**
     * @brief Reverts a future push (recorded by onFuturePush)
     * @param pusher Identifies the push to revert based on its originating square and direction (angle)
     */
    void undoFuturePush( Piece* pusher );

    /**
     * @brief undoes the last future move if safe to do so
     */
    void undoLastMove();

    /**
     * @brief Obtain the set of pieces representing differences between the current board and
     * what the board will be as a result of applying outstanding moves
     * @return set of future pieces
     */
    const PieceSet* getDeltaPieces();

    /**
     * @brief Get controlled access to the path finder
     */
    PathFinderController* getPathFinderController();

public slots:
    /**
     * @brief Receives notification that the tank is about to move toward the identified square
     * @param col The column of the target square
     * @param row The column of the target square
     * @param fromAngle The direction the tank will move in
     */
    void onTankMovingInto( int col, int row, int fromAngle );

    /**
     * @brief Receives notification that the board's map changed
     */
    void onBoardLoaded();

    /**
     * @brief Receives notification that a tile on the board changed.
     * @param col The identifying row where the change occurred
     * @param row The identifying column where the change occurred
     */
    void onBoardTileChanged( int col, int row );

    /**
     * @brief Recieves notification that a push has completed
     * @param type The type of piece being pushed
     * @param col The column pushed into
     * @param row The row pushed into
     * @param pieceAngle The orientation of the piece
     */
    void onPushed( PieceType type, int col, int row, int pieceAngle );

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

private:
    /**
     * @brief Determines whether the given single move is legal
     * @param what The type of peice being moved
     * @param angle The direction to move in. Must be one of 0, 90, 180 or 270.
     * @param col The Piece's originating column. Returns the resultant column.
     * @param row The Piece's originating row. Returns the resultant row.
     * @param board The board to consider
     * @param pushResult Set to true if a piece would get pushed by the move
     * @return true if allowed, otherwise false
     */
    bool canMoveFrom(PieceType what, int angle, int *col, int *row, Board* board, bool *pushResult = 0 );

    /**
     * @brief Determines whether the given piece can enter the given square.
     * @param what The type of peice.
     * @param col The column of the square to consider
     * @param row The row of the square to consider
     * @param fromAngle The entry direction
     * @param board The board to consider
     * @param pushResult true if the entry would result in pushing a piece
     * @return true if the entry is legal
     */
    bool canPlaceAt( PieceType what, int col, int row, int fromAngle, Board* board, bool *pushResult = 0 );

    SpeedController mSpeedController;
    AnimationStateAggregator mMoveAggregate;
    AnimationStateAggregator mShotAggregate;

    GameHandle mHandle;
    BoardWindow* mWindow;

    Board mBoard;
    PathFinderController mPathFinderController;
    Push mTankPush;
    Push mShotPush;
    Tank mTank;
    Shooter mActiveCannon;
    Board mFutureBoard;
    BoardDelta mFutureDelta;
};

#endif // GAME_H

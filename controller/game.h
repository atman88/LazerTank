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
 * @brief The Game class
 * Implements game logic and state
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
     * The single piece currently being pushed.
     * It's type is NONE when no piece is being pushed.
     */
    Push& getMovingPiece();

    /**
     * @brief the current lazer shot being fired.
     * Only one cannon is fired at a time. This is the single lazer path shot from any single selected cannon.
     */
    ShotModel& getCannonShot();

    /**
     * @brief Manages the current speed used by all animations.
     */
    SpeedController* getSpeedController();

    /**
     * @brief Holds the aggregate state of the current tank move including any piece the move is pushing.
     */
    AnimationStateAggregator* getMoveAggregate();

    /**
     * @brief Holds the aggregate state of active shots including any piece the shot is pushing.
     * @return
     */
    AnimationStateAggregator* getShotAggregate();

    /**
     * @brief Determines whether the given single move is legal
     * @param what The type of peice being moved
     * @param angle The direction to move in. Must be one of 0, 90, 180 or 270.
     * @param col The Piece's originating column as input. Returns the resultant column.
     * @param row The Piece's originating row as input. Returns the resultant row.
     * @param futuristic If true, all outstanding moves are considered, otherwise they are ignored.
     * @param pushResult Set to true if a piece would get pushed by the move
     * @return true if allowed, otherwise false
     */
    bool canMoveFrom(PieceType what, int angle, int *col, int *row, bool futuristic, bool* pushResult = 0 );

    /**
     * @brief Determines the outcome of a lazer shot advancing one square
     * @param angle The direction the lazer is travelling
     * @param col The column the lazer is advancing from. Returns the resultant column.
     * @param row The row the lazer is advancing from. Returns the resultant row.
     * @param endOffset Returns he offset (in pixels) within the target square where the lazer path terminated
     * @param source The shot being advanced
     * @return true if the the shot is continuing to advance or false if the shot hit something
     */
    bool advanceShot(int *angle, int *col, int *row, int *endOffset, ShotModel* source );

    /**
     * @brief Determines whether the given piece can enter the given square. Pending moves are not considered.
     * @param what The type of peice.
     * @param col The column of the square to consider
     * @param row The row of the square to consider
     * @param fromAngle The entry direction
     * @param pushResult true if the entry would result in pushing a piece
     * @return true if the entry is legal
     */
    bool canPlaceAtNonFuturistic(PieceType what, int col, int row, int fromAngle, bool *pushResult = 0);

    /**
     * @brief Records a push of piece that will be pushed as a result of some future change
     * @param pushingPiece The piece being pushed
     */
    void onFuturePush( Piece* pushingPiece );

    /**
     * @brief Reverts a future push (recorded by onFuturePush).
     * @param pusher Identifies the push to revert based on its originating square and direction (angle)
     */
    void undoFuturePush( Piece* pusher );

    /**
     * @brief Obtain the set of pieces which represent any differences between the current board and
     * what the board will be when outstanding moves are applied.
     * @return set of future pieces
     */
    const PieceSet* getDeltaPieces();

    /**
     * @brief Access to the path finder
     */
    PathFinderController* getPathFinderController();

public slots:
    /**
     * @brief Recieves notification that the tank is now in the given square
     * @param col The column of the square
     * @param row The row of the square
     */
    void onTankMoved( int col, int row );

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
     * @brief Receives aggregate changes of moving state
     */
    void onMovingPieceChanged(QAbstractAnimation::State newState, QAbstractAnimation::State oldState);
    void sightCannons();
    void endMoveDeltaTracking();

private:
    /**
     * @brief Determines the outcome of a lazer shot in the given square
     * @param col The column of the lazer end point
     * @param row The row of the lazer end point
     * @param angle the direction of the lazer
     * @param endOffset Returns he offset (in pixels) within the target square where the lazer path terminated
     * @param source The lazer beam being produced
     * @return true if the the shot is continuing to advance past the square or false if the shot hit something
     */
    bool canShootThru( int col, int row, int *angle, int *endOffset, ShotModel* source );

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
    Push mMovingPiece;
    Tank mTank;
    Shooter mActiveCannon;
    Board mFutureBoard;
    BoardDelta mFutureDelta;
};

#endif // GAME_H

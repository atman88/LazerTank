#ifndef MOVECONTROLLER_H
#define MOVECONTROLLER_H

#include <QObject>

class GameRegistry;
class Game;
class FutureShotPathManager;
class Board;
class PathSearchAction;

#include "model/futureshotpath.h"
#include "model/piecelistmanager.h"
#include "util/recorder.h"

typedef enum {
    Idle,
    MovingStage,
    FiringStage,
    IdlingStage
} MoveState;

class MoveBaseController : public QObject, public RecorderConsumer
{
    Q_OBJECT
public:
    explicit MoveBaseController( QObject* parent = 0 );
    virtual void init( GameRegistry* registry );
    virtual void onBoardLoaded( Board* board );

    /**
     * @brief move the tank one square
     * @param direction A rotation angle (one of 0, 90, 180, 270) or -1 to advance in the current direction
     */
    virtual void move( int direction, bool doWakeup = true );

    /**
     * @brief Retrieve the current focus vector coordinate
     */
    ModelVector getFocusVector() const;

     /**
     * @brief Access the pending tank moves
     */
    PieceListManager& getMoves();

    /**
     * @brief undoes the last future move if safe to do so
     */
    void undoLastMove();

    /**
     * @brief Access the pending tank shots
     */
    FutureShotPathManager& getFutureShots();

    /**
     * @brief Returns whether movement should occur
     * @return true if paused otherwise false
     */
    virtual bool canWakeup();

signals:
    /**
     * @brief Notifies the tank is about to push into an occupied square
     * @param point The new square
     * @param curRotation The direction of the move
     */
    void pushingInto( ModelPoint point, int curRotation );

    /**
     * @brief Notifies that the tank has completed all it's outstanding moves
     */
    void idle();

public slots:
    /**
     * @brief Fire the tank's laser
     * @param count The number of times to shoot or -1 to increment the shot count
     */
    void fire( int count = -1 ) override;

    /**
     * @brief Evaluate/advance the current state if possible
     */
    void wakeup();

    /**
     * @brief Cancel any pending (future) moves
     */
    void undoMoves();

    /**
     * @brief Retrieve the current focus state
     * @return MOVE if focus on the last move (future perspective) or TANK to focus on the first move
     * (present perspective)
     */
    PieceType getFocus() const;

    /**
     * @brief move the focus between the moves (future) and the tank (present)
     * @param what Either TANK to set the focus to the tank, otherwise focus is set to the moves
     */
    void setFocus( PieceType what );

protected slots:
    /**
     * @brief adds the given path to the tank's moves as appropriate
     * @param path The path to add
     * @param action The action which started the search
     */
    void onPathFound( PieceListManager* path, PathSearchAction* action );

protected:
    /**
     * @brief helper method to add a move to the list of moves that is highlight-aware
     * @param vector The column, row and direction for the new move
     * @param pushPiece The piece that this move pushes or 0 if it doesn't cause a push
     */
    void appendMove(ModelVector vector, Piece* pushPiece = 0 );

    /**
     * @brief Undoes the last move assuming state checking and highlight awareness is handled by the caller
     */
    void undoLastMoveInternal();

    /**
     * @brief change the move state
     * @param newState The state to transition to
     */
    void transitionState( MoveState newState );

    /**
     * @brief The square that the tank is moving toward. Nullified when not moving.
     */
    ModelVector mToVector;

    /**
     * @brief Pending moves where the first element may be in progress; all subsequent elements are future moves.
     */
    PieceListManager mMoves;

    /**
     * @brief Shots related with the pending moves (mMoves).
     */
    FutureShotPathManager mFutureShots;

    MoveState mState;
    PieceType mFocus;
};

typedef enum {
    Inactive,
    Searching,
    Selecting,
    Forbidden
} DragState;

class MoveDragController : public MoveBaseController
{
    Q_OBJECT
public:
    explicit MoveDragController( QObject* parent = 0 );
    void init( GameRegistry* registry ) override;
    void onBoardLoaded( Board* board ) override;

    /**
     * @brief Returns whether movement should occur
     * @return true if paused otherwise false
     */
    virtual bool canWakeup() override;

    /**
     * @brief Retrieve the currsor associated with the current state
     * @return The current cursor or 0 if not started
     */
    DragState getDragState() const;

    /**
     * @brief Handles a drag to the given point
     * @param p The point dragged to
     */
    void onDragTo( ModelPoint p );

    /**
     * @brief Begin a drag activity
     * @param startPoint The initial point of the drag
     */
    void dragStart( ModelPoint startPoint );

    /**
     * @brief move the tank one square
     * @param direction A rotation angle (one of 0, 90, 180, 270) or -1 to advance in the current direction
     * @param doWakeup Suppresses starting animations when false
     * Attempts to move beyond the current square are ignored when dragging is active
     */
    virtual void move( int direction, bool doWakeup = true ) override;


signals:
    /**
     * @brief Indicates that a change of state occured
     */
    void dragStateChanged( DragState state );

public slots:
    /**
     * @brief Completes the outstanding drag activity if any
     */
    void dragStop();

protected slots:
    /**
     * @brief Listens to path events for starting drag activities when self-initiated
     */
    void onPathFound( PieceListManager* path, PathSearchAction* action );

private:
    void setDragState( DragState state );

    DragState mDragState;
    bool mChanged;
};

class MoveController : public MoveDragController
{
    Q_OBJECT
public:
    explicit MoveController( QObject* parent = 0 );

    /**
     * @brief Returns whether movement should occur
     * @return true if paused otherwise false
     */
    bool canWakeup() override;

signals:
    /**
     * @brief Indicates that replay playback has ended
     */
    void replayFinished();

public slots:
    /**
     * @brief Enable/disable automatic replay
     * @param on
     */
    void setReplay( bool on ) override;

    /**
     * @brief Query whether automatic replay is enabled
     * @return true if enabled otherwise false
     */
    bool replaying() const;

private slots:
    void replayPlayback();

private:
    RecorderReader* mReplaySource;
};

#endif // MOVECONTROLLER_H

#ifndef MOVECONTROLLER_H
#define MOVECONTROLLER_H

#include <QObject>

class GameRegistry;
class Game;
class FutureShotPathManager;
class Board;
class PathSearchAction;
class BoardWindow;

#include "pathsearchcriteria.h"
#include "pathfindercontroller.h"
#include "model/futureshotpath.h"
#include "model/piecelistmanager.h"
#include "util/recorder.h"

typedef enum {
    Idle,
    MovingStage,
    FiringStage,
    IdlingStage
} MoveState;

class MoveBaseController : public QObject, public RecorderPlayer
{
    Q_OBJECT
public:
    explicit MoveBaseController( QObject* parent = 0 );

    virtual void init( GameRegistry* registry );
    virtual void onBoardLoaded( Board& board );

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
    virtual void undoLastMove();

    /**
     * @brief Access the pending tank shots
     */
    FutureShotPathManager& getFutureShots();

    /**
     * @brief Returns whether movement should occur
     * @return true if paused otherwise false
     */
    virtual bool canWakeup();

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
    virtual void undoMoves();

    /**
     * @brief focus-aware undo
     */
    virtual void undo();

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
    virtual void setFocus( PieceType what );

protected slots:
    /**
     * @brief adds the given path to the tank's moves as appropriate
     * @param path The path to add
     * @param criteria The parameters which started the search
     */
    void onPathFound( PieceListManager* path, PathSearchCriteria* criteria );

protected:
    /**
     * @brief helper method to add a move to the list of moves that is highlight-aware
     * @param moves The list to append to
     * @param vector The column, row and direction for the new move
     * @param pushPiece The piece that this move pushes or 0 if it doesn't cause a push
     */
    void appendMove( PieceListManager& moves, ModelVector vector, Piece* pushPiece = 0 );

    /**
     * @brief Extend or replace our path with the given path
     * @param path The new path segment
     * @param criteria The criteria used to find the new path
     * @return true if successful
     */
    bool applyPathUsingCriteria( PieceListManager* path, PathSearchCriteria* criteria );

    void moveInternal( const ModelVector& origin, PieceListManager& moves, int direction, bool lastMoveBusy );

    bool fireInternal( ModelVector initialVector, PieceListManager& moves, int count );

    /**
     * @brief Undoes the last move assuming state checking and highlight awareness is handled by the caller
     */
    void undoLastMoveInternal( PieceListManager& moves );

    /**
     * @brief change the move state
     * @param newState The state to transition to
     */
    void transitionState( MoveState newState );

    /**
     * @brief Invoked when idled
     */
    virtual void onIdle() = 0;

    /**
     * @brief The square that the tank is moving toward. Nullified when not moving.
     */
    ModelVector mToVector;

    /**
     * @brief Pending moves where the first element may be in progress; all subsequent elements are future moves.
     */
    PieceListManager mMoves;

    /**
     * @brief Shots related with the pending moves (mMoves)
     */
    FutureShotPathManager mFutureShots;

    MoveState mState;
    PieceType mFocus;
};

typedef enum {
    Inactive,
    Searching,
    DraggingTank,
    DraggingTile,
    Forbidden,     // attempting to start dragging to an unreachable location
    ForbiddenTank, // attempting to drag the tank to an illegal location
    ForbiddenTile, // attempting to drag a tile to an illegal location
} DragState;

class MoveDragController : public MoveBaseController
{
    Q_OBJECT
public:
    explicit MoveDragController( QObject* parent = 0 );

    void init( GameRegistry* registry ) override;
    void onBoardLoaded( Board& board ) override;

    /**
     * @brief Retrieve the currsor associated with the current state
     * @return The current cursor or 0 if not started
     */
    DragState getDragState() const;

    /**
     * @brief Retrieve the current drag focus vector
     */
    ModelVector getDragFocusVector() const;

    /**
     * @brief Handles a drag to the given point
     * @param coord The pixel coordinate dragged to
     */
    void onDragTo( QPoint coord );

    /**
     * @brief Begin a drag activity
     * @param startPoint The initial point of the drag
     */
    void dragStart( ModelPoint startPoint );

    /**
     * @brief Get the possible approach angles to the tile selected for dragging, as a bit mask
     */
    unsigned getDragTileAngleMask() const;

    /**
     * @brief Get the point for the tile being dragged
     * @return The square which the target tile resides on. Note the value is undetermined if a tile drag is not active
     */
    ModelPoint getDragTilePoint() const;

    /**
     * @brief Get the currently selected angle
     * @return An exit angle or -1 if no angle selected
     */
    int getTileDragFocusAngle() const;

    /**
     * @brief move the tank one square
     * @param direction A rotation angle (one of 0, 90, 180, 270) or -1 to advance in the current direction
     * @param doWakeup Suppresses starting animations when false
     * Attempts to move beyond the current square are ignored when dragging is active
     */
    void move( int direction, bool doWakeup = true ) override;

    /**
     * @brief undoes the last future move if safe to do so
     */
    void undoLastMove() override;

    /**
     * @brief Cancel any pending (future) moves
     */
    void undoMoves() override;

    /**
     * @brief focus-aware undo
     */
    void undo() override;

    /**
     * @brief move the focus between the moves (future) and the tank (present)
     * @param what Either TANK to set the focus to the tank, otherwise focus is set to the moves
     */
    void setFocus( PieceType what ) override;

    /**
     * @brief Get the push id boundary
     * @return The push id below the minimum value used for dragging, or -1 if dragging is not active
     */
    int getPushIdDelineation() const;

public slots:
    /**
     * @brief Fire the tank's laser
     * @param count The number of times to shoot or -1 to increment the shot count
     */
    void fire( int count = -1 ) override;

signals:
    /**
     * @brief Indicates that a change of state occured
     */
    void dragStateChanged( DragState state );

    /**
     * @brief Notifies when the angle selection changes
     * @param focusAngle A 90 degree angle value or -1 if nothing selected
     */
    void tileDragFocusChanged( int focusAngle );

    /**
     * @brief Notify that push pieces whose id is greater than delineation have changed
     * @param delineation pushId watermark value
     */
    void invalidatePushIdDelineation( int delineation );

public slots:
    /**
     * @brief Completes the outstanding drag activity if any
     */
    void dragStop();

protected slots:
    /**
     * @brief Listens to path events for starting drag activities when self-initiated
     */
    void onPathFound( PieceListManager* path, PathSearchCriteria* criteria );

    /**
     * @brief Listens for tile drag test results
     * @param reachable true if the target tile can be dragged in one or more directions
     * @param criteria The parameters tested
     */
    void onTestResult( bool reachable, PathSearchCriteria* criteria );

protected:
    PieceListManager mDragMoves;

private:
    void setDragState( DragState state );
    bool setTileDragFocusAngle( int angle );
    int getLastUsedPushId() const;

    DragState mDragState;
    QPoint mPreviousCoord;
    PathSearchCriteria mTileDragTestCriteria;
    TileDragTestResult mTileDragTestResult;
    int mTileDragFocusAngle;
    unsigned mTileDragAngleMask;
    int mMinPushedId;
    bool mChanged;
};

class MoveController : public MoveDragController
{
    Q_OBJECT
public:
    explicit MoveController( QObject* parent = 0 );
    ~MoveController();

    /**
     * @brief Returns whether movement should occur
     * @return true if paused otherwise false
     */
    bool canWakeup() override;

    /**
     * @brief Query whether automatic replay is enabled
     * @return true if enabled otherwise false
     */
    bool replaying() const;

    void render( Piece* pos, const QRect* rect, BoardRenderer& renderer, QPainter* painter );

    void connectWindow( const BoardWindow* window ) const;

signals:
    /**
     * @brief Notifies that the tank has completed all it's outstanding moves
     */
    void idle();

    /**
     * @brief Indicates that replay playback has ended
     */
    void replayFinished();

public slots:
    /**
     * @brief Enable/disable automatic replay
     * @param on Enables replay if true otherwise disables replay
     * @return previous on/off replay setting
     */
    bool setReplay( bool on ) override;

private slots:
    void replayPlayback();

private:
    RecorderReader* mReplayReader;

protected:
    void onIdle();
};

#endif // MOVECONTROLLER_H

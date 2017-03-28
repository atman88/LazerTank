#ifndef MOVECONTROLLER_H
#define MOVECONTROLLER_H

#include <QObject>

class Game;
class PathSearchAction;

#include "model/modelpoint.h"
#include "model/futureshotpath.h"
#include "model/piecelistmanager.h"


class MoveController : public QObject
{
    Q_OBJECT
public:
    explicit MoveController( QObject *parent = 0 );
    void init( Game* game );
    void reset();

    /**
     * @brief move the tank one square
     * @param direction A rotation angle (one of 0, 90, 180, 270) or -1 to evaluate the current move state (wake it up)
     */
    void move( int direction );

     /**
     * @brief Access the pending tank moves
     */
    PieceListManager* getMoves();

    /**
     * @brief Method to erase (undo) the last move. (Provides highlight-awareness)
     */
    void eraseLastMove();

    /**
     * @brief Access the pending tank shots
     */
    FutureShotPathManager* getFutureShots();

signals:
    /**
     * @brief Notifies the tank is about to push into an occupied square
     * @param col The column of the new square
     * @param row The row of the new square
     * @param curRotation The direction of the move
     */
    void pushingInto( int col, int row, int curRotation );

    /**
     * @brief Notifies that the tank has completed all it's outstanding moves
     */
    void idle();

public slots:
    /**
     * @brief Fire the tank's laser
     * @param count The number of times to shoot or -1 to increment the shot count
     */
    void fire( int count = -1 );

    /**
     * @brief Evaluate/advance the current state if possible
     */
    void wakeup();

    /**
     * @brief Cancel any pending (future) moves
     */
    void clearMoves();

    /**
     * @brief move the focus between the moves (future) and the tank (present)
     * @param what Either TANK to set the focus to the tank, otherwise focus is set to the moves
     */
    void setFocus( PieceType what );

    /**
     * @brief adds the given path to the tank's moves as appropriate
     * @param path The path to add
     * @param action The action which started the search
     */
    void onPathFound( PieceListManager* path, PathSearchAction* action );

private:
    /**
     * @brief helper method to add a move to the list of moves that is highlight-aware
     * @param col The column of the new move to append
     * @param row The row of the new move to append
     * @param direction The direction of the new move to append
     * @param shotCount The number of shots attributed to this move
     * @param pushPiece The piece that this move pushes or 0 if it doesn't cause a push
     */
    void appendMove( int col, int row, int direction, int shotCount = 0, Piece* pushPiece = 0 );

    /**
     * @brief The square that the tank is moving toward. Nullified when not moving.
     */
    ModelPoint mToPoint;

    /**
     * @brief The angle that the tank is rotating toward or -1 if not rotating.
     */
    int mToDirection;

    /**
     * @brief Pending moves where the first element may be in progress; all subsequent elements are future moves.
     */
    PieceListManager mMoves;

    /**
     * @brief Shots related with the pending moves (mMoves).
     */
    FutureShotPathManager mFutureShots;

    bool mIdle;
};

#endif // MOVECONTROLLER_H

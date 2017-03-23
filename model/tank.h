#ifndef TANK_H
#define TANK_H

#include <QObject>
#include <QPainter>
#include <QPropertyAnimation>

#include "controller/speedcontroller.h"
#include "controller/pathsearchaction.h"
#include "view/tankview.h"
#include "model/piecelistmanager.h"
#include "futureshotpath.h"

class Game;

class Tank : public TankView
{
    Q_OBJECT

public:
    Tank(QObject *parent = 0);
    virtual ~Tank() {}
    void init( Game* game );

    /**
     * @brief Access the pending tank moves
     */
    PieceListManager* getMoves();

    /**
     * @brief Access the pending tank shots
     */
    FutureShotPathManager* getFutureShots();

    /**
     * @brief move the tank one square
     * @param direction A rotation angle (one of 0, 90, 180, 270) or -1 to do the next pending move
     */
    void move( int direction );

    /**
     * @brief Fire the tank's laser
     * @param count The number of times to shoot or -1 to increment the shot count
     */
    void fire( int count = -1 );

    /**
     * @brief Get the column the tank currently resides on.
     * In the case where the tank is moving to a new column, this is the column it is moving away from.
     */
    int getCol() const;

    /**
     * @brief Get the row the tank currently resides on.
     * In the case where the tank is moving to a new row, this is the row it is moving away from.
     */
    int getRow() const;

    /**
     * @brief Get the direction the tank is facing
     * If the tank is currently rotating, this is the starting angle which is always the last 90 degree facing
     * @return The effective rotation. One of 0,90,180,270.
     */
    int getRotation() const;

    /**
     * @brief Helper method to erase (undo) the last move with highlight-awareness
     */
    void eraseLastMove();

signals:
    /**
     * @brief Notifies the tank is about to move to a new square
     * @param col The column of the new square
     * @param row The row of the new square
     * @param curRotation The direction of the move
     */
    void movingInto( int col, int row, int curRotation );

    /**
     * @brief Notifies that the tank has completed all it's outstanding moves
     */
    void idle();

public slots:
    /**
     * @brief Restores the tank to its idle state positioned at the given square
     * @param col The column of the square to position the tank in
     * @param row The row of the square to position the tank in
     */
    void reset( int col, int row );

    /**
     * @brief start the next pending move (if any)
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
     * @brief Aggregated animation slot handler
     */
    void onAnimationsFinished();

    /**
     * @brief adds the given path to the tank's moves as appropriate
     * @param path The path to add
     * @param action The action which started the search
     */
    void onPathFound( PieceListManager* path, PathSearchAction* action );

    /**
     * @brief Continue execution of the current move as appropriate
     */
    void resumeMove();

protected:
    /**
     * @brief internal move notification from the underlying view
     */
    void onMoved( int col, int row, int rotation ) override;

private:
    /**
     * @brief start a move
     * @param col The destination column
     * @param row The destination row
     * @param direction The direction to face the tank
     */
    void doMove( int col, int row, int direction );

    /**
     * @brief continue the current move
     * @param shotCount
     * @param move if created or 0 if adding shots for the current move state
     */
    void continueMove( int shotCount, Piece* move = 0 );

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
     * @brief Query if shot(s) need to complete before any next move can start
     */
    bool waitingOnShots();

    int mCol;
    int mRow;
    int mRotation;
    PieceListManager mMoves;
    FutureShotPathManager mFutureShots;
    bool mBusyFiring;
};

#endif // TANK_H

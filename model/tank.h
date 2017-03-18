#ifndef TANK_H
#define TANK_H

#include <QObject>
#include <QPainter>
#include <QPropertyAnimation>

#include "controller/speedcontroller.h"
#include "controller/pathsearchaction.h"
#include "view/tankview.h"
#include "model/piecelistmanager.h"

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
     * @brief move the tank one square
     * @param direction A rotation angle (one of 0, 90, 180, 270) or -1 to do the next pending move
     */
    void move( int direction );

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

signals:
    /**
     * @brief Notifies the tank is about to move to a new square
     * @param col The column of the new square
     * @param row The row of the new square
     * @param curRotation The direction of the move
     */
    void movingInto( int col, int row, int curRotation );

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
     * @brief Aggregated animation slot handler
     */
    void onAnimationsFinished();

     * @param doWakeup wake up movement animation if true
    void onPathFound( PieceListManager* path, bool doWakeup );
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

    int mCol;
    int mRow;
    int mRotation;
    PieceListManager mMoves;
};

#endif // TANK_H

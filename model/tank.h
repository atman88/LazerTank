#ifndef TANK_H
#define TANK_H

#include <QObject>
#include <QPainter>
#include <QPropertyAnimation>

#include "controller/speedcontroller.h"
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
     * @param direction A rotation angle. One of 0, 90, 180, 270.
     */
    void move( int direction = -1 );

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

signals:
    /**
     * @brief Notifies the tank has moved to a new square.
     * @param col Column of the new square.
     * @param row Row of the new square.
     */
    void moved( int col, int row );

    /**
     * @brief Notifies the thank is about to move to a new square
     * @param col The column of the new square
     * @param row The row of the new square
     * @param curRotation The direction of the move
     */
    void movingInto( int col, int row, int curRotation );
    void idled();

public slots:
    /**
     * @brief Restores the tank to its idle state positioned at the given square
     * @param col The column of the square to position the tank in
     * @param row The row of the square to position the tank in
     */
    void reset( int col, int row );

    /**
     * @brief Cancels any pending (future) moves
     */
    void clearMoves();

    /**
     * @brief Aggregated animation slot handler
     */
    void onAnimationsFinished();

protected:
    /**
     * @brief internal move notification from the underlying view
     */
    void onMoved( int col, int row ) override;

private:
    /**
     * @brief Instructs the tank to start the next pending move
     */
    void followPath();

    int mCol;
    int mRow;
    PieceListManager mMoves;
    bool mInReset;
};

#endif // TANK_H

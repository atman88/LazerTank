#ifndef TANK_H
#define TANK_H

#include <QObject>
#include <QPainter>
#include <QPropertyAnimation>

#include "view/tankview.h"

class Game;

class Tank : public TankView
{
    Q_OBJECT

public:
    Tank(QObject *parent = 0);
    virtual ~Tank() {}
    void init( Game* game );

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
     * @brief Animate to the given square and to the given rotation
     * @param col The square's column to animate into
     * @param row The square's row to animate into
     * @param direction The rotation to animate into
     * @return true if started animating
     */
    bool doMove( int col, int row, int direction );

public slots:
    /**
     * @brief Restores the tank to its idle state positioned at the given square
     * @param col The column of the square to position the tank in
     * @param row The row of the square to position the tank in
     */
    void reset( int col, int row );

protected:
    /**
     * @brief internal move notification from the underlying view
     */
    void onMoved( int col, int row, int rotation ) override;

private:
    int mCol;
    int mRow;
    int mRotation;
};

#endif // TANK_H

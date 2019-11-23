#ifndef TANK_H
#define TANK_H

#include "view/tankview.h"

class Game;
class PieceListManager;

class Tank : public TankView
{
    Q_OBJECT

public:
    Tank( QObject *parent = nullptr );
    virtual ~Tank() override = default;
    void init( GameRegistry* registry );

    /**
     * @brief Get the board position the tank currently resides on.
     * In the case where the tank is moving to a new square, the square the tank is moving away from is returned.
     */
    const ModelPoint& getPoint() const;

    /**
     * @brief Get the board position and rotation of the tank
     * In the case where the tank is moving to a new square, the square the tank is moving away from is returned.
     */
    const ModelVector& getVector() const;

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
    bool doMove( ModelVector& vector );

    bool fire();

public slots:
    /**
     * @brief Restores the tank to its idle state as appropriate after a board change
     * @param startVector The initial tank position
     */
    void onBoardLoaded( Board& board );

protected:
    /**
     * @brief internal move notification from the underlying view
     */
    void onMoved( int col, int row, int rotation ) override;

    ModelVector mVector;
};

#endif // TANK_H

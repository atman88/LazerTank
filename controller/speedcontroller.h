#ifndef SPEEDCONTROLLER_H
#define SPEEDCONTROLLER_H

#include <QObject>
#include <QPropertyAnimation>

/**
 * @brief A game animation speed container
 */
class SpeedController : public QObject
{
    Q_OBJECT
public:
    explicit SpeedController(QObject *parent = 0);

    /**
     * @brief get the current speed value in ms per square
     */
    int getSpeed();

    /**
     * @brief Get the high speed setting
     * @return true if high speed is on otherwise false
     */
    bool getHighSpeed();

public slots:
    /**
     * @brief Set the speed value
     * @param on sets high speed on if true, otherwise normal speed
     */
    void setHighSpeed( bool on );

signals:
    void speedChanged( int speed );

private:
    int mSpeed;
};

/**
 * @brief An animation which reacts to speed changes
 */
class SpeedControlledAnimation : public QPropertyAnimation
{
    Q_OBJECT

public:
    SpeedControlledAnimation( QObject* parent = 0 ) : QPropertyAnimation(parent)
    {
    }

    /**
     * @brief Specify a speed controller to react to
     * @param controller The controller to react to
     */
    void setController( SpeedController *controller );

    /**
     * @brief Helper method to start an animation between a given range
     * @param from The starting value
     * @param to The ending value
     */
    void animateBetween( int from, int to );

    /**
     * @brief Get the divisor. A divisor is the distance to travel in one second.
     * @return
     */
    virtual int getDivisor() = 0;

public slots:
    void setSpeed( int speed );

signals:
    void speedChanged( int speed );

protected:
    SpeedController* mController;
};

class MoveSpeedControlledAnimation : public SpeedControlledAnimation
{
    Q_OBJECT
public:
    MoveSpeedControlledAnimation( QObject* parent = 0 ) : SpeedControlledAnimation(parent)
    {
    }

    virtual int getDivisor();
};

class RotateSpeedControlledAnimation : public SpeedControlledAnimation
{
    Q_OBJECT
public:
    RotateSpeedControlledAnimation( QObject* parent = 0 ) : SpeedControlledAnimation(parent)
    {
    }

    virtual int getDivisor();
    void animateBetween( int fromAngle, int toAngle );
};

#endif // SPEEDCONTROLLER_H

#ifndef SPEEDCONTROLLER_H
#define SPEEDCONTROLLER_H

#include <QObject>
#include <QPropertyAnimation>

class Game;

/**
 * @brief A game animation speed container
 */
class SpeedController : public QObject
{
    Q_OBJECT

public:
    // movement speed in milliseconds per square
    static const int NormalSpeed = 800;
    static const int HighSpeed   = 300;

    explicit SpeedController(QObject *parent = nullptr);

    /**
     * @brief get the current speed value in ms per square
     */
    int getSpeed();

    /**
     * @brief Get the high speed setting
     * @return true if high speed is on otherwise false
     */
    bool getHighSpeed() const;

    /**
     * @brief Instruct this controller to subsequently re-evaluate the game speed
     */
    void stepSpeed();

public slots:
    /**
     * @brief Set the high speed override value
     * @param on sets high speed on if true, otherwise normal speed
     */
    void setHighSpeed( bool on );

    /**
     * @brief toggle high speed on/off
     */
    void toggleHighSpeed();

signals:
    void highSpeedChanged( int speed );

private:
    int desiredSpeed();

    bool mHighSpeed;
    bool mStepPending;
    int mSpeed;
};

/**
 * @brief An animation which reacts to speed changes
 */
class SpeedControlledAnimation : public QPropertyAnimation
{
    Q_OBJECT

public:
    SpeedControlledAnimation( QObject* parent = nullptr ) : QPropertyAnimation(parent), mController(nullptr)
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
     * @return true if the animation is started
     */
    bool animateBetween( int from, int to );

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
    MoveSpeedControlledAnimation( QObject* parent = nullptr ) : SpeedControlledAnimation(parent)
    {
    }

    int getDivisor() override;
};

class RotateSpeedControlledAnimation : public SpeedControlledAnimation
{
    Q_OBJECT
public:
    RotateSpeedControlledAnimation( QObject* parent = nullptr ) : SpeedControlledAnimation(parent)
    {
    }

    int getDivisor() override;
    bool animateBetween( int fromAngle, int toAngle );
};

#endif // SPEEDCONTROLLER_H

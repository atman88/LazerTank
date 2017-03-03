#ifndef SPEEDCONTROLLER_H
#define SPEEDCONTROLLER_H

#include <QObject>
#include <QPropertyAnimation>

// movement speed in ms/square
#define LOW_SPEED   1000
#define HIGH_SPEED   500

class SpeedController : public QObject
{
    Q_OBJECT
public:
    explicit SpeedController(QObject *parent = 0);

    int getSpeed();

public slots:
    void setSpeed( int speed );

signals:
    void speedChanged( int speed );

private:
    int mSpeed;
};

class SpeedControlledAnimation : public QPropertyAnimation
{
    Q_OBJECT
public:
    SpeedControlledAnimation( QObject* parent = 0 ) : QPropertyAnimation(parent)
    {
    }

    void setController( SpeedController *controller );
    void animateBetween( int from, int to );
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

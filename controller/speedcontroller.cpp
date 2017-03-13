#include <iostream>
#include "speedcontroller.h"

// movement speed in milliseconds per square
#define LOW_SPEED   800
#define HIGH_SPEED  400

SpeedController::SpeedController(QObject *parent) : QObject(parent), mSpeed(LOW_SPEED)
{
}

int SpeedController::getSpeed()
{
    return mSpeed;
}

bool SpeedController::getHighSpeed()
{
    return mSpeed == HIGH_SPEED;
}

void SpeedController::setHighSpeed( bool on )
{
    int speed = on ? HIGH_SPEED : LOW_SPEED;
    if ( speed != mSpeed ) {
        mSpeed = speed;
        emit speedChanged( speed );
    }
}

void SpeedControlledAnimation::setController(SpeedController *controller)
{
    mController = controller;
    QObject::connect( controller, &SpeedController::speedChanged, this, &SpeedControlledAnimation::setSpeed );
}

void SpeedControlledAnimation::setSpeed(int speed)
{
    if ( state() == QPropertyAnimation::Running ) {
        int newDuration = abs(endValue().toInt() - currentValue().toInt()) * speed / getDivisor();
        if ( newDuration > 0 ) {
            setDuration( newDuration );
        }
    }
}

int MoveSpeedControlledAnimation::getDivisor()
{
    return 24;
}

int RotateSpeedControlledAnimation::getDivisor()
{
    return 90;
}

void SpeedControlledAnimation::animateBetween( int from, int to )
{
    if ( from != to && mController ) {
        stop();
        setStartValue( from );
        setEndValue( to );
        setDuration( abs( to-from ) * mController->getSpeed() / getDivisor() );
        start();
    }
}

void RotateSpeedControlledAnimation::animateBetween( int fromAngle, int toAngle )
{
    fromAngle %= 360;
    if ( fromAngle == 0 && toAngle > 180 ) {
        fromAngle = 360;
    } else if ( toAngle == 0 && fromAngle > 180 ) {
        toAngle = 360;
    }
    SpeedControlledAnimation::animateBetween( fromAngle, toAngle );
}

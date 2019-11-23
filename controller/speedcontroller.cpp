#include <iostream>
#include <algorithm>

#include "speedcontroller.h"
#include "gameregistry.h"
#include "movecontroller.h"

constexpr int NSpeedSteps  = (SpeedController::NormalSpeed - SpeedController::HighSpeed)/100;

SpeedController::SpeedController(QObject* parent) : QObject(parent), mHighSpeed{false}, mStepPending{false}, mSpeed{NormalSpeed}
{
}

void SpeedController::stepSpeed()
{
    mStepPending = true;
}

int SpeedController::desiredSpeed()
{
    if ( mHighSpeed ) {
        return HighSpeed;
    }

    if ( GameRegistry* registry = getRegistry(this) ) {
        int distance = registry->getMoveController().getMoves().getList().size();
        return (distance < 3) ? NormalSpeed : NormalSpeed - std::min( distance-3, NSpeedSteps ) * 100;
    }

    return NormalSpeed;
}

int SpeedController::getSpeed()
{
    if ( mStepPending ) {
        int desired = desiredSpeed();
        if      ( mSpeed < desired ) { mSpeed += 100; }
        else if ( mSpeed > desired ) { mSpeed -= 100; }
        mStepPending = false;
    }
    return mSpeed;
}

bool SpeedController::getHighSpeed() const
{
    return mHighSpeed;
}

void SpeedController::setHighSpeed( bool on )
{
    if ( on != mHighSpeed ) {
        mHighSpeed = on;
        mSpeed = on ? HighSpeed : NormalSpeed;

        emit highSpeedChanged( getSpeed() );
    }
}

void SpeedController::toggleHighSpeed()
{
    setHighSpeed( !mHighSpeed );
}

void SpeedControlledAnimation::setController(SpeedController *controller)
{
    mController = controller;
    QObject::connect( controller, &SpeedController::highSpeedChanged, this, &SpeedControlledAnimation::setSpeed );
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

bool SpeedControlledAnimation::animateBetween( int from, int to )
{
    if ( from != to && mController ) {
        stop();
        setStartValue( from );
        setEndValue( to );
        setDuration( abs( to-from ) * mController->getSpeed() / getDivisor() );
        start();
        return true;
    }
    return false;
}

bool RotateSpeedControlledAnimation::animateBetween( int fromAngle, int toAngle )
{
    fromAngle %= 360;
    if ( fromAngle == 0 && toAngle > 180 ) {
        fromAngle = 360;
    } else if ( toAngle == 0 && fromAngle > 180 ) {
        toAngle = 360;
    }
    return SpeedControlledAnimation::animateBetween( fromAngle, toAngle );
}

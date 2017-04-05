#include <iostream>
#include <QEvent>
#include <QCoreApplication>
#include "tank.h"
#include "controller/game.h"
#include "controller/pathsearchaction.h"
#include "util/imageutils.h"
#include "util/renderutils.h"
#include "util/gameutils.h"

Tank::Tank(QObject* parent) : TankView(parent), mVector(0,0,0)
{
}

void Tank::init( Game* game )
{
    setParent(game);
    AnimationStateAggregator* aggregate = game->getMoveAggregate();
    QObject::connect( &mRotateAnimation,     &QPropertyAnimation::stateChanged, aggregate, &AnimationStateAggregator::onStateChanged );
    QObject::connect( &mHorizontalAnimation, &QPropertyAnimation::stateChanged, aggregate, &AnimationStateAggregator::onStateChanged );
    QObject::connect( &mVerticalAnimation,   &QPropertyAnimation::stateChanged, aggregate, &AnimationStateAggregator::onStateChanged );
    SpeedController* controller = game->getSpeedController();
    mRotateAnimation.setController( controller );
    mHorizontalAnimation.setController( controller );
    mVerticalAnimation.setController( controller );
    TankView::init( game );
}

const ModelPoint& Tank::getPoint() const
{
    return mVector;
}

const ModelVector& Tank::getVector() const
{
    return mVector;
}

void Tank::reset( ModelPoint point )
{
    mVector.setPoint( point );
    mVector.mAngle = 0;
    TankView::reset( point.toViewUpperLeft() );
    mRecorder.reset();
    std::cout << "tank: reset" << std::endl;
}

bool Tank::doMove( ModelVector& vector )
{
    bool moving =  mHorizontalAnimation.animateBetween( getViewX().toInt(), vector.mCol*24 )
                || mVerticalAnimation.animateBetween(   getViewY().toInt(), vector.mRow*24 );
    bool rotating = mRotateAnimation.animateBetween( mViewRotation, vector.mAngle );
    if ( moving || rotating ) {
        mRecorder.recordMove( moving, rotating ? vector.mAngle : -1 );
        return true;
    }
    return false;
}

bool Tank::fire()
{
    if ( Shooter::fire() ) {
        std::cout << "fire" << std::endl;
        mRecorder.recordShot();
        std::cout << "tank: firing " << getRotation() << std::endl;
        return true;
    }
    return false;
}

int Tank::getRow() const
{
    return mVector.mRow;
}

int Tank::getRotation() const
{
    return mVector.mAngle;
}

int Tank::getCol() const
{
    return mVector.mCol;
}

void Tank::onMoved( int col , int row, int rotation )
{
    mVector.mCol   = col;
    mVector.mRow   = row;
    mVector.mAngle = rotation;
}

Recorder& Tank::getRecorder()
{
    return mRecorder;
}

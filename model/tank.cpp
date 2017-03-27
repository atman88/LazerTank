#include <iostream>
#include <QEvent>
#include <QCoreApplication>
#include "tank.h"
#include "controller/game.h"
#include "controller/pathsearchaction.h"
#include "util/imageutils.h"
#include "util/renderutils.h"
#include "util/gameutils.h"

Tank::Tank(QObject* parent) : TankView(parent), mCol(0), mRow(0)
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

void Tank::reset( int col, int row )
{
    mCol = col;
    mRow = row;
    mRotation = 0;
    QPoint p( col*24, row*24 );
    TankView::reset( p );
}

bool Tank::doMove( int col, int row, int direction )
{
    return mRotateAnimation.animateBetween( mViewRotation, direction )
         | mHorizontalAnimation.animateBetween( getViewX().toInt(), col*24 )
         | mVerticalAnimation.animateBetween(   getViewY().toInt(), row*24 );
}

int Tank::getRow() const
{
    return mRow;
}

int Tank::getRotation() const
{
    return mRotation;
}

int Tank::getCol() const
{
    return mCol;
}

void Tank::onMoved(int col, int row, int rotation)
{
    mCol = col;
    mRow = row;
    mRotation = rotation;
}

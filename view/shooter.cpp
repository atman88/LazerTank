#include "shooter.h"
#include "boardwindow.h"
#include "game.h"

Shooter::Shooter(QObject *parent) : QObject(parent), mRotation(0), mBoundingRect( QRect(0,0,24,24) )
{
}

void Shooter::init( Game* game, QColor color )
{
    mShot.setParent(this);
    mShot.setColor( color );

    BoardWindow* window = game->getWindow();
    QObject::connect( &mShot, &ShotModel::tankKilled, window, &BoardWindow::onTankKilled );
    QObject::connect( &mShot, &ShotView::rectDirty,   window, &BoardWindow::renderLater );

    mShot.init( game->getShotAggregate() );
}

void Shooter::reset( QPoint p )
{
    mRotation = 0;
    mBoundingRect.moveTopLeft( p );
    mShot.reset();
}

void Shooter::fire()
{
    mShot.fire( this );
}

void Shooter::ceaseFire()
{
    mShot.startShedding();
}

QVariant Shooter::getX()
{
    return QVariant(mBoundingRect.left());
}

QVariant Shooter::getY()
{
    return QVariant(mBoundingRect.top());
}

const QRect& Shooter::getRect()
{
    return mBoundingRect;
}

void Shooter::setX( const QVariant& x )
{
    mBoundingRect.moveLeft( x.toInt() );
}

void Shooter::setY( const QVariant& y )
{
    mBoundingRect.moveTop( y.toInt() );
}

ShotModel& Shooter::getShot()
{
    return mShot;
}

QVariant Shooter::getRotation()
{
    return mRotation;
}

void Shooter::setRotation( const QVariant& angle )
{
    mRotation = angle;
}

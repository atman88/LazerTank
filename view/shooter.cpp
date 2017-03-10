#include "shooter.h"
#include "boardwindow.h"
#include "game.h"

Shooter::Shooter(QObject *parent) : QObject(parent), mViewRotation(0), mBoundingRect( QRect(0,0,24,24) )
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
    mViewRotation = 0;
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

QVariant Shooter::getViewX() const
{
    return QVariant(mBoundingRect.left());
}

QVariant Shooter::getViewY() const
{
    return QVariant(mBoundingRect.top());
}

const QRect& Shooter::getRect()
{
    return mBoundingRect;
}

void Shooter::setViewX( const QVariant& x )
{
    mBoundingRect.moveLeft( x.toInt() );
}

void Shooter::setViewY( const QVariant& y )
{
    mBoundingRect.moveTop( y.toInt() );
}

ShotModel& Shooter::getShot()
{
    return mShot;
}

QVariant Shooter::getViewRotation() const
{
    return QVariant(mViewRotation);
}

void Shooter::setViewRotation( const QVariant& angle )
{
    mViewRotation = angle.toInt();
}

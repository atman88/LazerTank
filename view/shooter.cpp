#include "shooter.h"

Shooter::Shooter(QObject *parent) : QObject(parent), mRotation(0), mBoundingRect( QRect(0,0,24,24) )
{
}

void Shooter::reset( QPoint& p )
{
    mRotation = 0;
    mBoundingRect.moveTopLeft( p );
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

QVariant Shooter::getRotation()
{
    return mRotation;
}

void Shooter::setRotation( const QVariant& angle )
{
    mRotation = angle;
}

#include <iostream>
#include "tank.h"
#include "controller/Game.h"

Tank::Tank( QObject *parent ) : QObject(parent)
{
    mPixmap.load(":/images/tank.png");
    mRotateAnimation = NULL;
    mRotation = 0;
    mRotateAnimation = new QPropertyAnimation(this,"rotation");
    mHorizontalAnimation = new QPropertyAnimation(this,"x");
    mVerticalAnimation   = new QPropertyAnimation(this,"y");
    QObject::connect(mRotateAnimation,    &QVariantAnimation::finished,this,&Tank::rotationAnimationFinished);
    QObject::connect(mVerticalAnimation,  &QVariantAnimation::finished,this,&Tank::moveAnimationFinished);
    QObject::connect(mHorizontalAnimation,&QVariantAnimation::finished,this,&Tank::moveAnimationFinished);
    mBoundingRect.setRect(0,0,24,24);
}

void Tank::paint( QPainter* painter )
{
    int x = mBoundingRect.left();
    int y = mBoundingRect.top();
    if ( mRotation != 0 ) {
        int centerX = x+24/2;
        int centerY = y+24/2;
        painter->translate( centerX, centerY );
        painter->rotate( mRotation.toDouble() );
        painter->translate(-centerX, -centerY);
    }
    painter->drawPixmap( x, y, mPixmap );
}

void Tank::onUpdate( int boardX, int boardY )
{
    QPoint p( boardX*24, boardY*24 );
    mHorizontalAnimation->stop();
    mVerticalAnimation->stop();
    mBoundingRect.moveTopLeft( p );
    mHorizontalAnimation->setStartValue( p.x() );
    mHorizontalAnimation->setEndValue( p.x() );
    mVerticalAnimation->setStartValue( p.y() );
    mVerticalAnimation->setEndValue( p.y() );
}

QVariant Tank::getX()
{
    return QVariant(mBoundingRect.left());
}

QVariant Tank::getY()
{
    return QVariant(mBoundingRect.top());
}

QRect* Tank::getRect()
{
    return &mBoundingRect;
}

void Tank::setX( const QVariant& x )
{
    int xv = x.toInt();
    if ( xv != mBoundingRect.left() ) {
        QRegion dirty( mBoundingRect );
        mBoundingRect.moveLeft( xv );
        dirty += mBoundingRect;
        emit changed( dirty.boundingRect() );
    }
}

void Tank::setY( const QVariant& y )
{
    int yv = y.toInt();
    if ( yv != mBoundingRect.top() ) {
        QRegion dirty( mBoundingRect );
        mBoundingRect.moveTop( yv );
        dirty += mBoundingRect;
        emit changed( dirty.boundingRect() );
    }
}

QVariant Tank::getRotation()
{
    return mRotation;
}

void Tank::setRotation( const QVariant& angle )
{
    if ( mRotation != angle ) {
        mRotation = angle;
        emit changed( mBoundingRect );
    }
}

bool Tank::isMoving()
{
    return mHorizontalAnimation->state() == QPropertyAnimation::Running
        || mVerticalAnimation->state()   == QPropertyAnimation::Running
        || mRotateAnimation->state()     == QPropertyAnimation::Running;
}

void Tank::rotationAnimationFinished()
{
    setRotation( QVariant( mRotation.toInt() % 360 ) );
    if ( !isMoving() ) {
        emit stopped();
    }
}

void Tank::move( int direction )
{
    int curRotation = mRotation.toInt();
    if ( direction != curRotation ) {
        mRotateAnimation->stop();

        if ( curRotation == 0 && direction > 180 ) {
            curRotation = 360;
            mRotateAnimation->setStartValue( QVariant(curRotation) );
        } else {
            mRotateAnimation->setStartValue( mRotation );
            if ( direction == 0 && curRotation > 180 ) {
                direction = 360;
            }
        }
        mRotateAnimation->setEndValue( QVariant(direction) );
        mRotateAnimation->setDuration( abs(direction-curRotation) * 1000 / 90);
        mRotateAnimation->start();
    } else {
        QObject* p = parent();
        QVariant hv = p->property("GameHandle");
        Game* game = hv.value<GameHandle>().game;
        if ( game && game->addMove( direction ) ) {
            QPropertyAnimation* animation;
            int startValue;
            int delta;

            switch( direction ) {
            case   0:
                animation = mVerticalAnimation;
                startValue = mBoundingRect.top();
                delta = -24;
                break;
            case  90:
                animation = mHorizontalAnimation;
                startValue = mBoundingRect.left();
                delta = 24;
                break;
            case 180:
                animation = mVerticalAnimation;
                startValue = mBoundingRect.top();
                delta = 24;
                break;
            case 270:
                animation = mHorizontalAnimation;
                startValue = mBoundingRect.left();
                delta = -24;
                break;
            default:
                return;
            }

            animation->stop();
            int endValue = animation->endValue().toInt() + delta;
            animation->setStartValue( startValue );
            animation->setEndValue( endValue );
            animation->setDuration(abs(endValue - startValue) * 1000 / 24);
            animation->start();
        }
    }
}

void Tank::moveAnimationFinished()
{
    if ( !isMoving() ) {
        emit stopped();
    }
}

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
    QObject::connect(mRotateAnimation,    &QVariantAnimation::finished,this,&Tank::animationFinished);
    QObject::connect(mVerticalAnimation,  &QVariantAnimation::finished,this,&Tank::animationFinished);
    QObject::connect(mHorizontalAnimation,&QVariantAnimation::finished,this,&Tank::animationFinished);
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

void Tank::reset( int boardX, int boardY )
{
    mRotateAnimation->stop();
    mHorizontalAnimation->stop();
    mVerticalAnimation->stop();
    QPoint p( boardX*24, boardY*24 );
    mBoundingRect.moveTopLeft( p );
    mHorizontalAnimation->setStartValue( p.x() );
    mVerticalAnimation->setStartValue( p.x() );
    mHorizontalAnimation->setEndValue( p.x() );
    mVerticalAnimation->setEndValue( p.x() );
    mMoves.clear();
}

QVariant Tank::getX()
{
    return QVariant(mBoundingRect.left());
}

QVariant Tank::getY()
{
    return QVariant(mBoundingRect.top());
}

const QRect& Tank::getRect()
{
    return mBoundingRect;
}

void Tank::setX( const QVariant& x )
{
    int xv = x.toInt();
    if ( xv != mBoundingRect.left() ) {
        QRect dirty( mBoundingRect );
        mBoundingRect.moveLeft( xv );
        dirty |= mBoundingRect;
        emit changed( dirty );

        if ( !(xv % 24) ) {
            emit moved( xv/24, mBoundingRect.top()/24 );
        }
    }
}

void Tank::setY( const QVariant& y )
{
    int yv = y.toInt();
    if ( yv != mBoundingRect.top() ) {
        QRect dirty( mBoundingRect );
        mBoundingRect.moveTop( yv );
        dirty |= mBoundingRect;
        emit changed( dirty );

        if ( !(yv % 24) ) {
            emit moved( mBoundingRect.left()/24, yv/24 );
        }
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

void Tank::move( int direction )
{
    int fromRotation;
    if ( !mMoves.size() ) {
        fromRotation = mRotation.toInt();
    } else {
        fromRotation = mMoves.back().getAngle();
    }
    if ( direction != fromRotation ) {
        if ( mMoves.empty() ) {
            mMoves.push_back( Piece( MOVE, mBoundingRect.left()/24, mBoundingRect.top()/24, direction ) );
        } else {
            PieceList::iterator it = mMoves.end();
            *--it = Piece( MOVE, it->getX(), it->getY(), direction );
            if ( mMoves.size() > 1 ) {
                emit pathAdded( mMoves.back() );
            }
        }
        if ( mMoves.size() == 1 ) {
            followPath();
        }
    } else {
        int x, y;
        if ( mMoves.empty() ) {
            x = mBoundingRect.left()/24;
            y = mBoundingRect.top()/24;
        } else {
            Piece& last = mMoves.back();
            x = last.getX();
            y = last.getY();
        }

        QObject* p = parent();
        QVariant hv = p->property("GameHandle");
        Game* game = hv.value<GameHandle>().game;

        if ( game->canMoveFrom( TANK, direction, &x, &y ) ) {
            mMoves.push_back( Piece( MOVE, x, y, direction ) );
            emit pathAdded( mMoves.back() );
            if ( mMoves.size() == 1 ) {
                followPath();
            }
        }
    }
}

bool Tank::followPath()
{
    if ( mMoves.empty() ) {
        return false;
    }

    Piece move( mMoves.front() );
    int curRotation = mRotation.toInt();
    int direction = move.getAngle();

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
        mRotateAnimation->setEndValue( direction );
        mRotateAnimation->setDuration( abs(direction-curRotation) * 1000 / 90);
        mRotateAnimation->start();
    }

    animateMove( mBoundingRect.left(), move.getX()*24, mHorizontalAnimation );
    animateMove( mBoundingRect.top(),  move.getY()*24, mVerticalAnimation   );

    return isMoving();
}

void Tank::animateMove( int from, int to, QPropertyAnimation *animation )
{
    int delta = to - from;
    if ( delta ) {
        animation->stop();
        animation->setStartValue( from );
        animation->setEndValue( to );
        animation->setDuration(abs(delta) * 1000 / 24);
        animation->start();
    }
}

bool Tank::isStopped()
{
    return mRotateAnimation->state()     == QPropertyAnimation::Stopped
        && mHorizontalAnimation->state() == QPropertyAnimation::Stopped
        && mVerticalAnimation->state()   == QPropertyAnimation::Stopped;
}

void Tank::animationFinished()
{
    if ( isStopped() ) {
        int rotation = mRotation.toInt() % 360;
        setRotation( QVariant( rotation ) );
        Piece& piece = mMoves.front();
        if ( piece.getAngle() == rotation
          && piece.getX() == mBoundingRect.left()/24
          && piece.getY() == mBoundingRect.top()/24 ) {
            mMoves.pop_front();
        }
        followPath();
    }
}

PieceList& Tank::getMoves()
{
    return mMoves;
}

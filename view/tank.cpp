#include <iostream>
#include <QEvent>
#include <QCoreApplication>
#include "tank.h"
#include "controller/Game.h"
#include "util/imageutils.h"

Tank::Tank(QObject* parent) : Shooter(parent)
{
    mRotateAnimation = new QPropertyAnimation(this,"rotation");
    mHorizontalAnimation = new QPropertyAnimation(this,"x");
    mVerticalAnimation   = new QPropertyAnimation(this,"y");
}

void Tank::init( Game* game )
{
    AnimationAggregator* aggregate = game->getMoveAggregate();
    QObject::connect( mRotateAnimation,     &QPropertyAnimation::stateChanged, aggregate, &AnimationAggregator::onStateChanged );
    QObject::connect( mHorizontalAnimation, &QPropertyAnimation::stateChanged, aggregate, &AnimationAggregator::onStateChanged );
    QObject::connect( mVerticalAnimation,   &QPropertyAnimation::stateChanged, aggregate, &AnimationAggregator::onStateChanged );
    QObject::connect( aggregate,            &AnimationAggregator::finished,    this,      &Tank::onAnimationsFinished          );
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
    painter->drawPixmap( x, y, *getPixmap(TANK) );
    if ( !painter->transform().isRotating() ) {
        mPreviousPaintRect = mBoundingRect;
    } else {
        mPreviousPaintRect = painter->transform().mapRect( mBoundingRect );
    }
}

void Tank::stop()
{
    mRotateAnimation->stop();
    mHorizontalAnimation->stop();
    mVerticalAnimation->stop();
}

void Tank::reset( int boardX, int boardY )
{
    stop();
    QPoint p( boardX*24, boardY*24 );
    mBoundingRect.moveTopLeft( p );
    mHorizontalAnimation->setStartValue( p.x() );
    mVerticalAnimation->setStartValue( p.x() );
    mHorizontalAnimation->setEndValue( p.x() );
    mVerticalAnimation->setEndValue( p.x() );
    mRotation = 0;
    mMoves.clear();

    emit moved( boardX, boardY );
}

void Tank::setX( const QVariant& x )
{
    int xv = x.toInt();
    if ( xv != mBoundingRect.left() ) {
        QRect dirty( mPreviousPaintRect );
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
        QRect dirty( mPreviousPaintRect );
        mBoundingRect.moveTop( yv );
        dirty |= mBoundingRect;
        emit changed( dirty );

        if ( !(yv % 24) ) {
            emit moved( mBoundingRect.left()/24, yv/24 );
        }
    }
}


void Tank::setRotation( const QVariant& angle )
{
    if ( mRotation != angle ) {
        mRotation = angle;
        emit changed( mPreviousPaintRect );
    }
}

void Tank::move( int direction )
{
    if ( direction < 0 ) {
        followPath();
        return;
    }

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
            --it;
            int x = it->getX();
            int y = it->getY();
            *it = Piece( MOVE, x, y, direction );
            if ( mMoves.size() > 1 ) {
                emit squareDirty( x, y );
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

        Game* game = getGame();
        if ( game && game->canMoveFrom( TANK, direction, &x, &y ) ) {
            mMoves.push_back( Piece( MOVE, x, y, direction ) );
            emit squareDirty( x, y );
            followPath();
        }
    }
}

void Tank::followPath()
{
    Game* game = getGame();
    if ( !game->getMoveAggregate()->active() && mMoves.size() ) {
        Piece move( mMoves.front() );
        int curRotation = mRotation.toInt();
        int direction = move.getAngle();
//    cout << "follow " << direction << " (" << move.getX() << "," << move.getY() << ")\n";

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

        int x = move.getX();
        int y = move.getY();
        animateMove( mBoundingRect.left(), x*24, mHorizontalAnimation );
        animateMove( mBoundingRect.top(),  y*24, mVerticalAnimation   );
        emit movingInto( x, y, curRotation % 360 );
    }
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

void Tank::onAnimationsFinished()
{
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

void Tank::eraseLastMove()\
{
    if ( mMoves.size() ) {
        Piece& piece = mMoves.back();
        emit squareDirty( piece.getX(), piece.getY() );
        mMoves.pop_back();
    }
}

PieceList& Tank::getMoves()
{
    return mMoves;
}

Game* Tank::getGame()
{
    QObject* p = parent();
    QVariant hv = p->property("GameHandle");
    return hv.value<GameHandle>().game;
}

void Tank::setMoves( PieceList moves )
{
    for( auto it : mMoves ) {
        emit squareDirty( it.getX(), it.getY() );
    }

    mMoves = moves;

    for( auto it : moves ) {
        emit squareDirty( it.getX(), it.getY() );
    }
}

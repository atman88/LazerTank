#include <iostream>
#include "shot.h"
#include "model/board.h"
#include "controller/Game.h"

Shot::Shot(QObject *parent) : QObject(parent)
{
    mAnimation = new QPropertyAnimation(this,"sequence");
    mAnimation->setStartValue(0);
    mAnimation->setEndValue(BOARD_MAX_WIDTH * BOARD_MAX_HEIGHT); // use a reasonable watermark value
    mAnimation->setDuration(60/8 * BOARD_MAX_WIDTH * BOARD_MAX_HEIGHT);
}

void Shot::init( AnimationAggregator* aggregate )
{
    QObject::connect( mAnimation, &QPropertyAnimation::stateChanged, aggregate, &AnimationAggregator::onStateChanged );
}

QVariant Shot::getSequence()
{
    return mSequence;
}

void Shot::setSequence( const QVariant &sequence )
{
    int seq = sequence.toInt();
    if ( !mEndReached ) {
        int direction, x, y;
        const PieceList* pathList = mPath.getList();
        if ( !pathList->size() ) {
            direction = mDirection;
            x = parent()->property( "x" ).toInt() / 24;
            y = parent()->property( "y" ).toInt() / 24;
        } else {
            Piece* p = pathList->back();
            direction = p->getAngle();
            x = p->getX();
            y = p->getY();
        }

        int startDir = direction;
        Game* game = getGame();
        if ( !game || !game->canShootFrom( &direction, &x, &y ) ) {
            mPath.append( SHOT_END, x, y, direction );
            mStopping = true;
            mEndReached = true;
        } else {
            PieceType pieceType;
            if ( direction - startDir == 90 || (direction == 0 && startDir == 270) ) {
                pieceType = SHOT_RIGHT;
            } else if ( direction - startDir == -90 || (direction == 270 && startDir == 0) ) {
                pieceType = SHOT_LEFT;
            } else {
                pieceType = SHOT_STRAIGHT;
            }
            mPath.append( pieceType, x, y, direction );
        }
    }
    if ( mStopping && seq > 5 ) {
        mPath.eraseFront();
    }
    if ( !mPath.count() ) {
        std::cout << "shot finished\n";
        mAnimation->stop();
    }
    mSequence = seq;
}

void Shot::stop()
{
    mStopping = true;
}

void Shot::fire( int direction )
{
    if ( !(direction % 90) ) {
        if ( mAnimation->state() == QPropertyAnimation::Stopped ) {
            mSequence = QVariant(-1);
            mDirection = direction;
            mStopping = false;
            mEndReached = false;
            mAnimation->start();
        }
    }
}

Game* Shot::getGame()
{
    // find the game from the object hierarchy:
    QObject* p = parent();
    QVariant v;
    while( p && !(v = p->property("GameHandle")).isValid() ) {
        p = p->parent();
    }
    return v.value<GameHandle>().game;
}

PieceListManager& Shot::getPath()
{
    return mPath;
}

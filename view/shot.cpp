#include <iostream>
#include "shot.h"

Shot::Shot(QObject *parent) : QObject(parent)
{
    mAnimation = new QPropertyAnimation(this,"sequence");
    mAnimation->setStartValue(0);
    mAnimation->setEndValue(BOARD_MAX_WIDTH * BOARD_MAX_HEIGHT); // use a reasonable watermark value
    mAnimation->setDuration(60/8 * BOARD_MAX_WIDTH * BOARD_MAX_HEIGHT);
}

void Shot::init( Game* game )
{
    QObject::connect( mAnimation, &QPropertyAnimation::stateChanged, game->getShotAggregate(), &AnimationAggregator::onStateChanged );
}

QVariant Shot::getSequence()
{
    return mSequence;
}

void Shot::setSequence( const QVariant &sequence )
{
    int seq = sequence.toInt();
    if ( mSequence.toInt() != seq ) {
        if ( !mEndReached ) {
            int direction, x, y;
            if ( !mPath.size() ) {
                direction = mDirection;
                x = parent()->property( "x" ).toInt() / 24;
                y = parent()->property( "y" ).toInt() / 24;
            } else {
                Piece& p = mPath.back();
                direction = p.getAngle();
                x = p.getX();
                y = p.getY();
            }

            int startDir = direction;
            Game* game = getGame();
            if ( !game || !game->canShootFrom( &direction, &x, &y ) ) {
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
                mPath.push_back( Piece(pieceType,x,y,direction) );
                emit pathAdded( mPath.back() );
            }
        }
        if ( mStopping && seq > 5 ) {
            if ( !mPath.empty() ) {
                Piece& piece = mPath.front();
                emit pathRemoved( piece );
                mPath.pop_front();
            }
        }
        mSequence = seq;
    }
    if ( mPath.empty() ) {
        std::cout << "shot finished\n";
        mAnimation->stop();
    }
}

void Shot::stop()
{
    mStopping = true;
}

void Shot::fire( int direction )
{
    if ( !(direction % 90) ) {
        Game* game = getGame();
        if ( game && !game->getShotAggregate()->active() ) {
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

PieceList& Shot::getPath()
{
    return mPath;
}

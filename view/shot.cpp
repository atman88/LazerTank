#include <iostream>
#include "shot.h"
#include "model/board.h"
#include "controller/Game.h"

Shot::Shot(QObject *parent) : QObject(parent), mDistance(0), mStopping(false), mEndReached(false), mKillSequence(0)
{
    mAnimation.setTargetObject(this);
    mAnimation.setPropertyName("sequence");
    mAnimation.setStartValue(0);
    mAnimation.setEndValue(BOARD_MAX_WIDTH * BOARD_MAX_HEIGHT); // use a reasonable watermark value
    mAnimation.setDuration(60/8 * BOARD_MAX_WIDTH * BOARD_MAX_HEIGHT);
}

void Shot::init( AnimationAggregator* aggregate )
{
    QObject::connect( &mAnimation, &QPropertyAnimation::stateChanged, aggregate, &AnimationAggregator::onStateChanged );
}

void Shot::reset()
{
    mSequence = QVariant(-1);
    mDistance = 0;
    mStopping = false;
    mEndReached = false;
    mKillSequence = 0;
    mPath.reset();
}

QVariant Shot::getSequence()
{
    return mSequence;
}

void Shot::setSequence( const QVariant &sequence )
{
    mSequence = sequence;

    Game* game = getGame();
    if ( !game ) {
        return;
    }

    if ( mKillSequence ) {
        switch( mKillSequence++ ) {
        case 1:
            mPath.replaceBack( SHOT_END_KILL );
            break;
        case 3:
            mAnimation.pause();
            emit tankKilled();
            break;
        default:
            break;
        }
        return;
    }

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
        int endOffset;
        if ( !game->canShootFrom( &direction, &x, &y, &endOffset, this ) ) {
            mPath.append( SHOT_END, x, y, direction, endOffset );
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

    if ( ++mDistance > 5 && mStopping && !mKillSequence ) {
        mPath.eraseFront();
    }

    if ( !mPath.count() ) {
        std::cout << "shot finished\n";
        mAnimation.stop();
    }
}

void Shot::stop()
{
    mStopping = true;
}

void Shot::setIsKill()
{
    mKillSequence = 1;
}

void Shot::fire( int direction )
{
    if ( !(direction % 90) ) {
        reset();
        mDirection = direction;
        mAnimation.start();
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

PieceListManager* Shot::getPath()
{
    return &mPath;
}

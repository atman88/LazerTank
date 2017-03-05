#include <iostream>

#include <QVariant>

#include "pathfinder.h"
#include "game.h"

// search map values:
#define TRAVERSIBLE 3
#define BLOCKED     4
#define TARGET      5

PathFinder::PathFinder(QObject *parent) : QThread(parent)
{
}

void PathFinder::findPath( int targetX, int targetY, int startingX, int startingY, int startingRotation )
{
    mStopping = true;

    mTargetX = targetX;
    mTargetY = targetY;
    mStartingX = startingX;
    mStartingY = startingY;
    mStartingRotation = startingRotation;
    start( LowPriority );
}

Game* PathFinder::getGame()
{
    // find the game from the object hierarchy:
    QObject* p = parent();
    QVariant v;
    while( p && !(v = p->property("GameHandle")).isValid() ) {
        p = p->parent();
    }
    return v.value<GameHandle>().game;
}

void PathFinder::printSearchMap()
{
    for( int y = 0; y <= mMaxY; ++y ) {
        for( int x = 0; x <= mMaxX; ++x ) {
            switch( mSearchMap[y * BOARD_MAX_WIDTH + x] ) {
            case 0:          cout << '0'; break;
            case 1:          cout << '1'; break;
            case 2:          cout << '2'; break;
            case TRAVERSIBLE:cout << ' '; break;
            case BLOCKED:    cout << 'X'; break;
            case TARGET:     cout << 'T'; break;
            default:         cout << '?'; break;
            }
        }
        cout << std::endl;
    }
}

void PathFinder::buildPath( int x, int y )
{
    int direction = 0;

    while( y != mTargetY || x != mTargetX ) {
        int lastX = x;
        int lastY = y;

        if ( --mPassValue < 0 ) {
            mPassValue = TRAVERSIBLE-1;
        }
        if      ( y > 0     && mSearchMap[ (y-1) * BOARD_MAX_WIDTH + x   ] == mPassValue ) { --y; direction =   0; }
        else if ( x > 0     && mSearchMap[ y     * BOARD_MAX_WIDTH + x-1 ] == mPassValue ) { --x; direction = 270; }
        else if ( y < mMaxY && mSearchMap[ (y+1) * BOARD_MAX_WIDTH + x   ] == mPassValue ) { ++y; direction = 180; }
        else if ( x < mMaxX && mSearchMap[ y     * BOARD_MAX_WIDTH + x+1 ] == mPassValue ) { ++x; direction =  90; }
        else {
            break;
        }

        if ( lastX != mStartingX || lastY != mStartingY || direction != mStartingRotation ) {
            mMoves.append( MOVE, lastX, lastY, direction );
        }
    }
    mMoves.append( MOVE, x, y, direction );
}

void PathFinder::tryAt( int x, int y )
{
    if ( x >= 0 && x <= mMaxX
      && y >= 0 && y <= mMaxY ) {
        switch( mSearchMap[y*BOARD_MAX_WIDTH + x] ) {
        case TRAVERSIBLE:
            mSearchMap[y*BOARD_MAX_WIDTH + x] = mPassValue;
            if ( mPushIndex >= 0 && mPushIndex < (int) ((sizeof mSearchX)/(sizeof *mSearchX)) ) {
                mSearchX[mPushIndex] = x;
                mSearchY[mPushIndex] = y;
                mPushIndex += mPushDirection;
            }
            break;

        case TARGET:
//            printSearchMap();
//
            buildPath( x, y );
            std::longjmp( mJmpBuf, 1 );

        default:
            ;
        }
    }
}

int PathFinder::pass1( int nPoints )
{
    mPassValue = (mPassValue + 1) % TRAVERSIBLE;
    mPushIndex = (sizeof mSearchX)/(sizeof *mSearchX)-1;
    mPushDirection = -1;

    for( int pullIndex = 0; pullIndex < nPoints && !mStopping; ++pullIndex ) {
        int x = mSearchX[pullIndex];
        int y = mSearchY[pullIndex];
        tryAt( x,   y-1 );
        tryAt( x-1, y   );
        tryAt( x,   y+1 );
        tryAt( x+1, y   );
    }
    return (sizeof mSearchX)/(sizeof *mSearchX) - mPushIndex;
}

int PathFinder::pass2( int nPoints )
{
    mPassValue = (mPassValue + 1) % TRAVERSIBLE;
    mPushIndex = 0;
    mPushDirection = 1;
    int endIndex = (sizeof mSearchX)/(sizeof *mSearchX) - nPoints;

    for( int pullIndex = (sizeof mSearchX)/(sizeof *mSearchX); --pullIndex > endIndex && !mStopping; ) {
        int x = mSearchX[pullIndex];
        int y = mSearchY[pullIndex];
        tryAt( x,   y-1 );
        tryAt( x-1, y   );
        tryAt( x,   y+1 );
        tryAt( x+1, y   );
    }
    return mPushIndex;
}

void PathFinder::run()
{
    mStopping = false;

    mMoves.reset();
    if ( !setjmp( mJmpBuf ) ) {
        // initialize the search map
        Game* game = getGame();
        if ( game && game->canPlaceAtNonFuturistic( TANK, mTargetX, mTargetY, 0 )) {
            Board* board = game->getBoard();
            mMaxX  = board->getWidth()-1;
            mMaxY = board->getHeight()-1;

            for( int y = mMaxY; !mStopping && y >= 0; --y ) {
                for( int x = mMaxX; !mStopping && x >= 0; --x ) {
                    mSearchMap[y*BOARD_MAX_WIDTH+x] = game->canPlaceAtNonFuturistic( TANK, x, y, 0 ) ? TRAVERSIBLE : BLOCKED;
                }
            }

            mSearchMap[mStartingY*BOARD_MAX_WIDTH+mStartingX] = TARGET;
            mPassValue = 0;
            mSearchMap[mTargetY*BOARD_MAX_WIDTH+mTargetX] = mPassValue;

            mSearchX[0] = mTargetX;
            mSearchY[0] = mTargetY;
            int nPoints = 1;
            while( nPoints > 0 && !mStopping ) {
                nPoints = pass1( nPoints );
                nPoints = pass2( nPoints );
            }
        }
    }
    emit pathFound( &mMoves );
}

#include <iostream>

#include <QVariant>

#include "pathfinder.h"
#include "Game.h"

// search map values:
#define TRAVERSIBLE 3
#define BLOCKED     4
#define TARGET      5

PathFinder::PathFinder(QObject *parent) : QThread(parent)
{
}

void PathFinder::findPath( int fromX, int fromY, int targetX, int targetY, int targetRotation )
{
    mStopping = true;

    mFromX = fromX;
    mFromY = fromY;
    mTargetX = targetX;
    mTargetY = targetY;
    mTargetRotation = targetRotation;
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
    for( int y = 0; y < mMapHeight; ++y ) {
        for( int x = 0; x < mMapWidth; ++x ) {
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

    while( y != mFromY || x != mFromX ) {
        int lastX = x;
        int lastY = y;

        if ( --mPassValue < 0 ) {
            mPassValue = TRAVERSIBLE-1;
        }
        if      ( mSearchMap[ (y-1) * BOARD_MAX_WIDTH + x   ] == mPassValue ) { --y; direction =   0; }
        else if ( mSearchMap[ y     * BOARD_MAX_WIDTH + x-1 ] == mPassValue ) { --x; direction = 270; }
        else if ( mSearchMap[ (y+1) * BOARD_MAX_WIDTH + x   ] == mPassValue ) { ++y; direction = 180; }
        else if ( mSearchMap[ y     * BOARD_MAX_WIDTH + x+1 ] == mPassValue ) { ++x; direction =  90; }
        else {
            break;
        }

        if ( lastX != mTargetX || lastY != mTargetY || direction != mTargetRotation ) {
            mMoves.push_back( Piece( MOVE, lastX, lastY, direction ) );
        }
    }
    mMoves.push_back( Piece( MOVE, x, y, direction ) );
}

void PathFinder::tryAt( int x, int y )
{
    if ( x >= 0 && x < mMapWidth
      && y >= 0 && y < mMapHeight ) {
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
            printSearchMap();

            mMoves.clear();
            buildPath( x, y );
            emit pathFound( mMoves );
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

    if ( !setjmp( mJmpBuf ) ) {
        // initialize the search map
        Game* game = getGame();
        if ( game ) {
            Board* board = game->getBoard();
            mMapWidth  = board->getWidth();
            mMapHeight = board->getHeight();

            for( int y = mMapHeight; !mStopping && --y >= 0; ) {
                for( int x = mMapWidth; !mStopping && --x >= 0; ) {
                    mSearchMap[y*BOARD_MAX_WIDTH+x] = game->canPlaceAt( TANK, x, y, 0, false ) ? TRAVERSIBLE : BLOCKED;
                }
            }

            mSearchMap[mTargetY*BOARD_MAX_WIDTH+mTargetX] = TARGET;
            mPassValue = 0;
            mSearchMap[mFromY*BOARD_MAX_WIDTH+mFromX] = mPassValue;

            mSearchX[0] = mFromX;
            mSearchY[0] = mFromY;
            int nPoints = 1;
            while( nPoints > 0 && !mStopping ) {
                nPoints = pass1( nPoints );
                nPoints = pass2( nPoints );
            }
        }
    }
}

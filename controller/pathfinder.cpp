#include <iostream>

#include <QVariant>

#include "util/gameutils.h"
#include "pathfinder.h"
#include "game.h"

// search map values:
#define TRAVERSIBLE 3
#define BLOCKED     4
#define TARGET      5

PathFinder::PathFinder(QObject *parent) : QThread(parent)
{
}

void PathFinder::findPath(int targetCol, int targetRow, int startCol, int startRow, int startRotation, bool testOnly )
{
    mStopping = true;

    mTargetCol = targetCol;
    mTargetRow = targetRow;
    mStartCol = startCol;
    mStartRow = startRow;
    mStartRotation = startRotation;
    mTestOnly = testOnly;
    start( LowPriority );
}

void PathFinder::printSearchMap()
{
    for( int row = 0; row <= mMaxRow; ++row ) {
        for( int col = 0; col <= mMaxCol; ++col ) {
            switch( mSearchMap[row * BOARD_MAX_WIDTH + col] ) {
            case 0:          std::cout << '0'; break;
            case 1:          std::cout << '1'; break;
            case 2:          std::cout << '2'; break;
            case TRAVERSIBLE:std::cout << ' '; break;
            case BLOCKED:    std::cout << 'X'; break;
            case TARGET:     std::cout << 'T'; break;
            default:         std::cout << '?'; break;
            }
        }
        std::cout << std::endl;
    }
}

void PathFinder::buildPath( int col, int row )
{
    int direction = 0;

    while( row != mTargetRow || col != mTargetCol ) {
        int lastCol = col;
        int lastRow = row;

        if ( --mPassValue < 0 ) {
            mPassValue = TRAVERSIBLE-1;
        }
        if      ( row > 0       && mSearchMap[(row-1) * BOARD_MAX_WIDTH + col  ] == mPassValue ) { --row; direction =   0; }
        else if ( col > 0       && mSearchMap[row     * BOARD_MAX_WIDTH + col-1] == mPassValue ) { --col; direction = 270; }
        else if ( row < mMaxRow && mSearchMap[(row+1) * BOARD_MAX_WIDTH + col  ] == mPassValue ) { ++row; direction = 180; }
        else if ( col < mMaxCol && mSearchMap[row     * BOARD_MAX_WIDTH + col+1] == mPassValue ) { ++col; direction =  90; }
        else {
            break;
        }

        if ( lastCol != mStartCol || lastRow != mStartRow || direction != mStartRotation ) {
            mMoves.append( MOVE, lastCol, lastRow, direction );
        }
    }
    mMoves.append( MOVE, col, row, direction );
}

void PathFinder::tryAt( int col, int row )
{
    if ( col >= 0 && col <= mMaxCol
      && row >= 0 && row <= mMaxRow ) {
        switch( mSearchMap[row*BOARD_MAX_WIDTH + col] ) {
        case TRAVERSIBLE:
            mSearchMap[row*BOARD_MAX_WIDTH + col] = mPassValue;
            if ( mPushIndex >= 0 && mPushIndex < (int) ((sizeof mSearchCol)/(sizeof *mSearchCol)) ) {
                mSearchCol[mPushIndex] = col;
                mSearchRow[mPushIndex] = row;
                mPushIndex += mPushDirection;
            }
            break;

        case TARGET:
            if ( !mTestOnly ) {
                buildPath( col, row );
            }
            std::longjmp( mJmpBuf, 1 );

        default:
            ;
        }
    }
}

int PathFinder::pass1( int nPoints )
{
    mPassValue = (mPassValue + 1) % TRAVERSIBLE;
    mPushIndex = (sizeof mSearchCol)/(sizeof *mSearchCol)-1;
    mPushDirection = -1;

    for( int pullIndex = 0; pullIndex < nPoints && !mStopping; ++pullIndex ) {
        int col = mSearchCol[pullIndex];
        int row = mSearchRow[pullIndex];
        tryAt( col,   row-1 );
        tryAt( col-1, row   );
        tryAt( col,   row+1 );
        tryAt( col+1, row   );
    }
    return (sizeof mSearchCol)/(sizeof *mSearchCol) - mPushIndex;
}

int PathFinder::pass2( int nPoints )
{
    mPassValue = (mPassValue + 1) % TRAVERSIBLE;
    mPushIndex = 0;
    mPushDirection = 1;
    int endIndex = (sizeof mSearchCol)/(sizeof *mSearchCol) - nPoints;

    for( int pullIndex = (sizeof mSearchCol)/(sizeof *mSearchCol); --pullIndex > endIndex && !mStopping; ) {
        int col = mSearchCol[pullIndex];
        int row = mSearchRow[pullIndex];
        tryAt( col,   row-1 );
        tryAt( col-1, row   );
        tryAt( col,   row+1 );
        tryAt( col+1, row   );
    }
    return mPushIndex;
}

void PathFinder::run()
{
    mStopping = false;
    Game* game = getGame(this);

    int startCol = mStartCol;
    int startRow = mStartRow;
    int startRotation = mStartRotation;
    int targetCol = mTargetCol;
    int targetRow = mTargetRow;

    mMoves.reset();
    bool found = false;
    switch( setjmp( mJmpBuf ) ) {
    case 0:
        // initialize the search map
        if ( game && game->canPlaceAtNonFuturistic( TANK, mTargetCol, mTargetRow, 0 )) {
            Board* board = game->getBoard();
            mMaxCol  = board->getWidth()-1;
            mMaxRow = board->getHeight()-1;

            for( int row = mMaxRow; !mStopping && row >= 0; --row ) {
                for( int col = mMaxCol; !mStopping && col >= 0; --col ) {
                    mSearchMap[row*BOARD_MAX_WIDTH+col] = game->canPlaceAtNonFuturistic( TANK, col, row, 0 ) ? TRAVERSIBLE : BLOCKED;
                }
            }

            mSearchMap[mStartRow*BOARD_MAX_WIDTH+mStartCol] = TARGET;
            mPassValue = 0;
            mSearchMap[mTargetRow*BOARD_MAX_WIDTH+mTargetCol] = mPassValue;

            mSearchCol[0] = targetCol;
            mSearchRow[0] = targetRow;
            int nPoints = 1;
            while( nPoints > 0 && !mStopping ) {
                nPoints = pass1( nPoints );
                nPoints = pass2( nPoints );
            }
        }
        break;

    default:
        found = true;
    }

    if ( mTestOnly ) {
        emit testResult( found, targetCol, targetRow, startCol, startRow, startRotation );
    } else if ( found && mMoves.size() > 0 ) {
        emit pathFound( targetCol, targetRow, startCol, startRow, startRotation, &mMoves );
    }
}

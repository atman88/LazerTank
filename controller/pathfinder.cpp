#include <iostream>
#include <QVariant>

#include "util/gameutils.h"
#include "pathfinder.h"
#include "controller/game.h"
#include "controller/gameregistry.h"
#include "model/push.h"

// search map values:
#define TRAVERSIBLE 3
#define BLOCKED     4
#define TARGET      5

PathFinder::PathFinder(QObject *parent) : QThread(parent), mStopping(false), mPassValue(0), mPushIndex(0), mPushDirection(0), mTestOnly(false)
{
}

bool PathFinder::findPath( PathSearchCriteria* criteria, bool testOnly )
{
    mStopping = true;

    // initialize the search map
    // Note: Done here (in the app thread) to ensure the board doesn't change while reading it.
    if ( GameRegistry* registry = getRegistry(this) ) {
        Game& game = registry->getGame();
        if ( game.canPlaceAt( TANK, criteria->getTargetPoint(), 0, criteria->isFuturistic() )) {
            Board* board = game.getBoard( criteria->isFuturistic() );
            mMaxPoint = board->getLowerRight();

            ModelPoint point;
            for( point.mRow = mMaxPoint.mRow; point.mRow >= 0; --point.mRow ) {
                for( point.mCol = mMaxPoint.mCol; point.mCol >= 0; --point.mCol ) {
                    mSearchMap[point.mRow*BOARD_MAX_WIDTH+point.mCol] =
                      game.canPlaceAt( TANK, point, 0, criteria->getFocus() != TANK ) ? TRAVERSIBLE : BLOCKED;
                }
            }

            // account for any outstanding pushes if this is on the master board
            if ( game.isMasterBoard(board) ) {
                addPush( registry->getTankPush() );
                addPush( registry->getShotPush() );
            }

            mCriteria = *criteria;
            mTestOnly = testOnly;
            start( LowPriority );
            return true;
        }
    }
    return false;
}

void PathFinder::addPush( Push& push )
{
    if ( push.getType() != NONE ) {
        mSearchMap[ push.getTargetPoint().mRow*BOARD_MAX_WIDTH + push.getTargetPoint().mCol ] = BLOCKED;
    }
}

/*
void PathFinder::printSearchMap()
{
    for( int row = 0; row <= mMaxPoint.mRow; ++row ) {
        for( int col = 0; col <= mMaxPoint.mCol; ++col ) {
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
*/

void PathFinder::buildPath( int col, int row )
{
    int direction = 0;

    while( col != mRunCriteria.getTargetCol() || row != mRunCriteria.getTargetRow() ) {
        ModelPoint lastPoint( col, row );

        if ( --mPassValue < 0 ) {
            mPassValue = TRAVERSIBLE-1;
        }
        if      ( row > 0              && mSearchMap[(row-1) * BOARD_MAX_WIDTH + col  ] == mPassValue ) { --row; direction =   0; }
        else if ( col > 0              && mSearchMap[row     * BOARD_MAX_WIDTH + col-1] == mPassValue ) { --col; direction = 270; }
        else if ( row < mMaxPoint.mRow && mSearchMap[(row+1) * BOARD_MAX_WIDTH + col  ] == mPassValue ) { ++row; direction = 180; }
        else if ( col < mMaxPoint.mCol && mSearchMap[row     * BOARD_MAX_WIDTH + col+1] == mPassValue ) { ++col; direction =  90; }
        else {
            break;
        }

        if ( lastPoint.mCol != mRunCriteria.getStartCol()
          || lastPoint.mRow != mRunCriteria.getStartRow()
          || direction != mRunCriteria.getStartDirection() ) {
            mMoves.append( MOVE, lastPoint, direction );
        }
    }
    mMoves.append( MOVE, ModelPoint( col, row ), direction );
}

void PathFinder::tryAt( int col, int row )
{
    if ( col >= 0 && col <= mMaxPoint.mCol
      && row >= 0 && row <= mMaxPoint.mRow ) {
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

    // copy the action for background thread use:
    mRunCriteria = mCriteria;

    mMoves.reset();
    bool found = false;
    switch( setjmp( mJmpBuf ) ) {
    case 0:
        mSearchMap[mRunCriteria.getStartRow()*BOARD_MAX_WIDTH+mRunCriteria.getStartCol()] = TARGET;
        mPassValue = 0;
        mSearchMap[mRunCriteria.getTargetRow()*BOARD_MAX_WIDTH+mRunCriteria.getTargetCol()] = mPassValue;

        mSearchCol[0] = mRunCriteria.getTargetCol();
        mSearchRow[0] = mRunCriteria.getTargetRow();
    {   int nPoints = 1;
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
        emit testResult( found, mRunCriteria );
    } else if ( found && mMoves.size() > 0 ) {
        emit pathFound( mRunCriteria, &mMoves );
    }
}

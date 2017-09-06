#include <iostream>
#include <QVariant>

#include "util/gameutils.h"
#include "pathfinder.h"
#include "controller/game.h"
#include "controller/pathfindercontroller.h"
#include "controller/gameregistry.h"
#include "model/push.h"

// search map values:
#define TRAVERSIBLE 3
#define BLOCKED     4
#define TARGET      5

PathFinder::PathFinder( QObject* parent ) : QObject(parent), mStopping(false), mNPoints(0), mPassValue(0), mPushIndex(0),
  mPushDirection(0), mTestOnly(false), mPathSearchRunnable(*this), mTileDragBuildRunnable(*this)
{
}

bool PathFinder::execCriteria( PathSearchCriteria* criteria, bool testOnly )
{
    mStopping = true;

    // initialize the search map
    // Note: Done here (in the app thread) to ensure the board doesn't change while reading it.
    if ( GameRegistry* registry = getRegistry(this) ) {
        Game& game = registry->getGame();
        Board* board = game.getBoard( criteria->isFuturistic() );
        mMaxPoint = board->getLowerRight();

        ModelPoint point;
        for( point.mRow = mMaxPoint.mRow; point.mRow >= 0; --point.mRow ) {
            for( point.mCol = mMaxPoint.mCol; point.mCol >= 0; --point.mCol ) {
                mSearchMap[point.mRow*BOARD_MAX_WIDTH+point.mCol] =
                  game.canPlaceAt( TANK, point, -1, criteria->getFocus() != TANK ) ? TRAVERSIBLE : BLOCKED;
            }
        }

        // account for any outstanding pushes if this is on the master board
        if ( game.isMasterBoard(board) ) {
            addPush( registry->getTankPush() );
            addPush( registry->getShotPush() );
        }

        mCriteria = *criteria;
        mTestOnly = testOnly;
        registry->getWorker().doWork( &mPathSearchRunnable );
        return true;
    }
    return false;
}

void PathFinder::addPush( Push& push )
{
    if ( push.getType() != NONE ) {
        mSearchMap[ push.getTargetPoint().mRow*BOARD_MAX_WIDTH + push.getTargetPoint().mCol ] = BLOCKED;
    }
}

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

#define CANBUILD() mSearchMap[row*BOARD_MAX_WIDTH+col]==mPassValue

bool PathFinder::buildPath()
{
    ModelVector curVector = mRunCriteria.getStartVector();

    int angle = curVector.mAngle;
    int col = curVector.mCol;
    int row = curVector.mRow;

    while( !mRunCriteria.getTargetPoint().equals( curVector ) ) {
        if ( --mPassValue < 0 ) {
            mPassValue = TRAVERSIBLE-1;
        }
        for( ;; ) {
            switch( angle ) {
            case   0: if ( row > 0              ) { --row; if ( CANBUILD() ) goto found; ++row; } break;
            case 270: if ( col > 0              ) { --col; if ( CANBUILD() ) goto found; ++col; } break;
            case 180: if ( row < mMaxPoint.mRow ) { ++row; if ( CANBUILD() ) goto found; --row; } break;
            case  90: if ( col < mMaxPoint.mCol ) { ++col; if ( CANBUILD() ) goto found; --col; } break;
            }

            angle = (angle + 90) % 360;
            if ( angle == curVector.mAngle ) {
                return false;
            }
        }
      found:
        curVector.mAngle = angle;
        if ( !curVector.equals( mRunCriteria.getStartVector() ) ) {
            mMoves.append( MOVE, curVector );
        }
        curVector.mCol = col;
        curVector.mRow = row;
    }
    mMoves.append( MOVE, curVector );
    return true;
}

bool PathFinder::buildTilePushPath( ModelVector target )
{
    if ( mRunCriteria.getTargetPoint().equals( target ) ) {
        if ( GameRegistry* registry = getRegistry(this) ) {
            mTileDragBuildRunnable.mTarget = target;
            registry->getWorker().doWork( &mTileDragBuildRunnable );
            return true;
        }
    }
    return false;
}

void PathFinder::buildTilePushPathInternal( ModelVector target )
{
    printSearchMap();

    mMoves.reset();
    ModelPoint endPoint = mRunCriteria.getStartVector();
    if ( endPoint.equals( target ) ) {
        emit pathFound( mRunCriteria, &mMoves );
        return;
    }

    ModelVector curVector( target );
    if ( getAdjacentPosition( (curVector.mAngle + 180) % 360, &curVector ) ) {
        mMoves.push_front( MOVE, curVector );

        int col = curVector.mCol;
        int row = curVector.mRow;
        mPassValue = mSearchMap[curVector.mRow*BOARD_MAX_WIDTH + curVector.mCol];

        while( !endPoint.equals( curVector ) ) {
            if ( --mPassValue < 0 ) {
                mPassValue = TRAVERSIBLE-1;
            }

            int angle = curVector.mAngle;
            for( ;; ) {
                switch( curVector.mAngle ) {
                case 180: if ( row > 0              ) { --row; if ( CANBUILD() ) goto found; ++row; } break;
                case  90: if ( col > 0              ) { --col; if ( CANBUILD() ) goto found; ++col; } break;
                case   0: if ( row < mMaxPoint.mRow ) { ++row; if ( CANBUILD() ) goto found; --row; } break;
                case 270: if ( col < mMaxPoint.mCol ) { ++col; if ( CANBUILD() ) goto found; --col; } break;
                }

                curVector.mAngle = (curVector.mAngle + 90) % 360;
                if ( curVector.mAngle == angle ) {
                    return;
                }
            }
          found:
            curVector.mCol = col;
            curVector.mRow = row;
            mMoves.push_front( MOVE, curVector );
        }
        emit pathFound( mRunCriteria, &mMoves );
    }
}

bool PathFinder::tryAt( int col, int row )
{
    if ( col >= 0 && col <= mMaxPoint.mCol
      && row >= 0 && row <= mMaxPoint.mRow ) {
        switch( mSearchMap[row*BOARD_MAX_WIDTH + col] ) {
        case TARGET:
        {   ModelPoint point( col, row );
            auto it = mTargets.find( point );
            if ( it != mTargets.end() ) {
                mTargets.erase( it );
                if ( TileDragTestResult* result = mRunCriteria.getTileDragTestResult() ) {
                    result->mPossibleApproaches.insert( point );
                }
                if ( mTargets.empty() ) {
                    mSearchMap[row*BOARD_MAX_WIDTH + col] = mPassValue;
                    return true;
                }
            }
        }

            // fall through

        case TRAVERSIBLE:
            mSearchMap[row*BOARD_MAX_WIDTH + col] = mPassValue;
            if ( mPushIndex >= 0 && mPushIndex < (int) ((sizeof mSearchCol)/(sizeof *mSearchCol)) ) {
                mSearchCol[mPushIndex] = col;
                mSearchRow[mPushIndex] = row;
                mPushIndex += mPushDirection;
            }
            break;

        default:
            ;
        }
    }
    return false;
}

void PathFinder::pass1()
{
    mPassValue = (mPassValue + 1) % TRAVERSIBLE;
    mPushIndex = (sizeof mSearchCol)/(sizeof *mSearchCol)-1;
    mPushDirection = -1;

    for( unsigned pullIndex = 0; pullIndex < mNPoints && !mStopping; ++pullIndex ) {
        int col = mSearchCol[pullIndex];
        int row = mSearchRow[pullIndex];
        if ( tryAt( col,   row-1 )
          || tryAt( col-1, row   )
          || tryAt( col,   row+1 )
          || tryAt( col+1, row   ) ) {
            std::longjmp( mJmpBuf, 1 );
        }
    }
    mNPoints = (sizeof mSearchCol)/(sizeof *mSearchCol) - mPushIndex;
}

void PathFinder::pass2()
{
    mPassValue = (mPassValue + 1) % TRAVERSIBLE;
    mPushIndex = 0;
    mPushDirection = 1;
    int endIndex = (sizeof mSearchCol)/(sizeof *mSearchCol) - mNPoints;

    for( int pullIndex = (sizeof mSearchCol)/(sizeof *mSearchCol); --pullIndex > endIndex && !mStopping; ) {
        int col = mSearchCol[pullIndex];
        int row = mSearchRow[pullIndex];
        if ( tryAt( col,   row-1 )
          || tryAt( col-1, row   )
          || tryAt( col,   row+1 )
          || tryAt( col+1, row   ) ) {
            std::longjmp( mJmpBuf, 1 );
        }
    }
    mNPoints = mPushIndex;
}

void PathFinder::doSearchInternal()
{
    mStopping = false;

    // copy the action for background thread use:
    mRunCriteria = mCriteria;

    mNPoints = 0;
    mPushIndex = 0;
    mPushDirection = 1;
    mMoves.reset();
    mTargets.clear();
    bool found = false;
    if ( !setjmp( mJmpBuf ) ) {
        mPassValue = 0;
        switch( mRunCriteria.getCriteriaType() ) {
        case PathSearchCriteria::PathCriteria:
            // For path search the starting point is targetted
            mSearchMap[mRunCriteria.getStartRow() *BOARD_MAX_WIDTH+mRunCriteria.getStartCol() ] = TARGET;
            mTargets.insert( mRunCriteria.getStartVector() );
            tryAt( mRunCriteria.getTargetCol(), mRunCriteria.getTargetRow() );
            // Note we are not interested in 0-length paths in this case so we don't set found here
            break;

        case PathSearchCriteria::TileDragTestCriteria:
            // for multi target test, the target points are targeted
            if ( TileDragTestResult* result = mRunCriteria.getTileDragTestResult() ) {
                for( auto it : result->mPossibleApproaches ) {
                    char* p = &mSearchMap[it.mRow*BOARD_MAX_WIDTH+it.mCol];
                    if ( *p == TRAVERSIBLE ) {
                        *p = TARGET;
                        mTargets.insert( it );
                    }
                }
                result->mPossibleApproaches.clear();
                tryAt( mRunCriteria.getStartCol(), mRunCriteria.getStartRow() );
            }
            break;

        default:
            ;
        }

        mNPoints = mPushIndex;
        while( mNPoints && !mStopping ) {
            pass1();
            pass2();
        }
    } else {
        found = true;
    }

    if ( mTestOnly ) {
        // At this point found is set when ALL are found. For the drag test, set found if ANY are found:
        if ( !found && mRunCriteria.getCriteriaType() == PathSearchCriteria::TileDragTestCriteria ) {
            if ( TileDragTestResult* result = mRunCriteria.getTileDragTestResult() ) {
                found = !result->mPossibleApproaches.empty();
            }
        }
        emit testResult( found, mRunCriteria );
    } else if ( found ) {
        if ( buildPath() ) {
            emit pathFound( mRunCriteria, &mMoves );
        }
    }
}

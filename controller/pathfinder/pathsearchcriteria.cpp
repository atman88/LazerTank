#include <iostream>
#include "pathsearchcriteria.h"
#include "gameregistry.h"
#include "game.h"
#include "movecontroller.h"
#include "model/tank.h"
#include "model/piece.h"

PathSearchCriteria::PathSearchCriteria() : mCriteriaType(NullCriteria), mFocus(MOVE), mAction(0), mTileDragTestResult(0)
{
}

PathSearchCriteria::PathSearchCriteria( const PathSearchCriteria& source ) : mCriteriaType(source.mCriteriaType),
  mStartVector(source.mStartVector), mTargetPoint(source.mTargetPoint), mFocus(source.mFocus), mAction(source.mAction),
  mTileDragTestResult(source.mTileDragTestResult)
{
}

bool PathSearchCriteria::setPathCriteria( PieceType focus, const ModelPoint& target, QAction* action )
{
    if ( GameRegistry* registry = getRegistry(action) ) {
        mCriteriaType = PathCriteria;
        mTargetPoint = target;
        setFocusInternal( focus, registry );
        mAction = action;
        mTileDragTestResult = 0;
        return true;
    }
    return false;
}

bool PathSearchCriteria::setTileDragCriteria( PieceType focus, const Piece* target, TileDragTestResult* result )
{
    if ( GameRegistry* registry = getRegistry(result) ) {
        mCriteriaType = TileDragTestCriteria;
        mTargetPoint = *target;
        mAction = 0;

        // prime the result here while we have the piece reference & we are running on the app thread
        result->mPossibleApproaches.clear();
        for( int angle = 0; angle < 360; angle += 90 ) {
            if ( Game::canPushPiece( target, angle ) ) {
                if ( registry->getGame().canPlaceAt( TANK, *target, angle, true ) ) {
                    ModelPoint point( *target );
                    if ( getAdjacentPosition( (angle + 180) % 360, &point ) ) {
                        result->mPossibleApproaches.insert( point );
                    }
                }
            }
        }

        if ( result->mPossibleApproaches.size() ) {
            setFocusInternal( focus, registry );
            mTileDragTestResult = result;
            return true;
        }
    }
    return false;
}

PathSearchCriteria& PathSearchCriteria::operator =(const PathSearchCriteria other)
{
    mCriteriaType       = other.mCriteriaType;
    mStartVector        = other.mStartVector;
    mTargetPoint        = other.mTargetPoint;
    mFocus              = other.mFocus;
    mAction             = other.mAction;
    mTileDragTestResult = other.mTileDragTestResult;
    return *this;
}

bool PathSearchCriteria::operator ==(const PathSearchCriteria other)
{
    return mCriteriaType       == other.mCriteriaType
         && mFocus              == other.mFocus
         && mAction             == other.mAction
         && mTileDragTestResult == other.mTileDragTestResult
         && mStartVector.equals( other.mStartVector )
         && mTargetPoint.equals( other.mTargetPoint );
}

PathSearchCriteria::CriteriaType PathSearchCriteria::getCriteriaType() const
{
    return mCriteriaType;
}

ModelPoint PathSearchCriteria::getTargetPoint() const
{
    return mTargetPoint;
}

int PathSearchCriteria::getTargetCol() const
{
    return mTargetPoint.mCol;
}

int PathSearchCriteria::getTargetRow() const
{
    return mTargetPoint.mRow;
}

ModelPoint PathSearchCriteria::getStartPoint() const
{
    return mStartVector;
}

ModelVector PathSearchCriteria::getStartVector() const
{
    return mStartVector;
}

int PathSearchCriteria::getStartCol() const
{
    return mStartVector.mCol;
}

int PathSearchCriteria::getStartRow() const
{
    return mStartVector.mRow;
}

PieceType PathSearchCriteria::getFocus() const
{
    return mFocus;
}

bool PathSearchCriteria::isFuturistic() const
{
    return mFocus != TANK;
}

void PathSearchCriteria::setFocusInternal( PieceType focus, GameRegistry* registry )
{
    if ( focus == TANK ) {
        mFocus = TANK;
    } else {
        Piece* move = registry->getMoveController().getMoves().getBack();
        if ( !move ) {
            mFocus = TANK;
        } else {
            mFocus = MOVE;
            mStartVector = *move;
        }
    }

    if ( mFocus == TANK ) {
        mStartVector = registry->getTank().getVector();
    }
}

TileDragTestResult* PathSearchCriteria::getTileDragTestResult() const
{
    return mTileDragTestResult;
}

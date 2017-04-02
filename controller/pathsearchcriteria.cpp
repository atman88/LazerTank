#include <iostream>
#include "pathsearchcriteria.h"

PathSearchCriteria::PathSearchCriteria() : mStartDirection(0), mFocus(MOVE), mMoveWhenFound(false)
{
}

PathSearchCriteria::PathSearchCriteria( const PathSearchCriteria& source ) : mStartPoint(source.mStartPoint),
  mStartDirection(source.mStartDirection), mTargetPoint(source.mTargetPoint), mFocus(source.mFocus),
  mMoveWhenFound(source.mMoveWhenFound)
{
}

PathSearchCriteria& PathSearchCriteria::operator =(const PathSearchCriteria other)
{
    mStartPoint     = other.mStartPoint;
    mStartDirection = other.mStartDirection;
    mTargetPoint    = other.mTargetPoint;
    mFocus          = other.mFocus;
    mMoveWhenFound  = other.mMoveWhenFound;

    return *this;
}

bool PathSearchCriteria::operator ==(const PathSearchCriteria other)
{
    return mStartPoint     == other.mStartPoint
        && mStartDirection == other.mStartDirection
        && mTargetPoint    == other.mTargetPoint
        && mFocus          == other.mFocus
        && mMoveWhenFound  == other.mMoveWhenFound;
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

bool PathSearchCriteria::getMoveWhenFound() const
{
    return mMoveWhenFound;
}

ModelPoint PathSearchCriteria::getStartPoint() const
{
    return mStartPoint;
}

int PathSearchCriteria::getStartCol() const
{
    return mStartPoint.mCol;
}

int PathSearchCriteria::getStartRow() const
{
    return mStartPoint.mRow;
}

int PathSearchCriteria::getStartDirection() const
{
    return mStartDirection;
}

PieceType PathSearchCriteria::getFocus() const
{
    return mFocus;
}

bool PathSearchCriteria::isFuturistic() const
{
    return mFocus != TANK;
}

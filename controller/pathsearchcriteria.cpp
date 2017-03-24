#include <iostream>
#include "pathsearchcriteria.h"

PathSearchCriteria::PathSearchCriteria() : mStartCol(-1), mStartRow(-1), mStartDirection(0), mTargetCol(-1), mTargetRow(-1),
  mFocus(MOVE), mMoveWhenFound(false)
{
}

PathSearchCriteria::PathSearchCriteria( const PathSearchCriteria& source ) : mStartCol(source.mStartCol),
  mStartRow(source.mStartRow), mStartDirection(source.mStartDirection), mTargetCol(source.mTargetCol),
  mTargetRow(source.mTargetRow), mFocus(source.mFocus), mMoveWhenFound(source.mMoveWhenFound)
{
}

PathSearchCriteria& PathSearchCriteria::operator =(const PathSearchCriteria other)
{
    mStartCol       = other.mStartCol;
    mStartRow       = other.mStartRow;
    mStartDirection = other.mStartDirection;
    mTargetCol      = other.mTargetCol;
    mTargetRow      = other.mTargetRow;
    mFocus          = other.mFocus;
    mMoveWhenFound  = other.mMoveWhenFound;

    return *this;
}

bool PathSearchCriteria::operator ==(const PathSearchCriteria other)
{
    return mStartCol       == other.mStartCol
        && mStartRow       == other.mStartRow
        && mStartDirection == other.mStartDirection
        && mTargetCol      == other.mTargetCol
        && mTargetRow      == other.mTargetRow
        && mFocus          == other.mFocus
        && mMoveWhenFound  == other.mMoveWhenFound;
}

int PathSearchCriteria::getTargetCol() const
{
    return mTargetCol;
}

int PathSearchCriteria::getTargetRow() const
{
    return mTargetRow;
}

bool PathSearchCriteria::getMoveWhenFound() const
{
    return mMoveWhenFound;
}

int PathSearchCriteria::getStartCol() const
{
    return mStartCol;
}

int PathSearchCriteria::getStartRow() const
{
    return mStartRow;
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

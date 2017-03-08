#include "pathsearchaction.h"
#include "pathfindercontroller.h"
#include "game.h"

PathSearchAction::PathSearchAction(QObject *parent) : QAction(parent), mTargetCol(-1), mTargetRow(-1), mMoveWhenFound(false)
{
}

void PathSearchAction::setCriteria( int targetCol, int targetRow, bool moveWhenFound )
{
    mTargetCol = targetCol;
    mTargetRow = targetRow;
    mMoveWhenFound = moveWhenFound;
}

int PathSearchAction::getTargetCol() const
{
    return mTargetCol;
}

int PathSearchAction::getTargetRow() const
{
    return mTargetRow;
}

bool PathSearchAction::getMoveWhenFound() const
{
    return mMoveWhenFound;
}

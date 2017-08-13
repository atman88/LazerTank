#include <iostream>
#include "pathfindercontroller.h"
#include "pathfinder.h"
#include "pathsearchaction.h"
#include "game.h"


PathFinderController::PathFinderController(QObject *parent) : QObject(parent), mCurAction(0)
{
}

void PathFinderController::init()
{
    mPathFinder.setParent(this);
    qRegisterMetaType<PathSearchCriteria>("PathSearchCriteria");
    QObject::connect( &mPathFinder, &PathFinder::testResult, this, &PathFinderController::onResult, Qt::QueuedConnection );
    if ( !((bool) QObject::connect( &mPathFinder, &PathFinder::pathFound,  this, &PathFinderController::onPath,   Qt::QueuedConnection )) ) {
        std::cout << "*** connection failed" << std::endl;
    }
}

bool PathFinderController::doAction( PathSearchAction* action, bool testOnly )
{
    mCurAction = action;
    return mPathFinder.findPath( action->getCriteria(), testOnly );
}

void PathFinderController::testNextAction()
{
    if ( mTestActions.size() > 0 ) {
        mCurAction = mTestActions.front();
        mTestActions.pop_front();
        doAction( mCurAction, true );
    }
}

void PathFinderController::testActions( PathSearchAction* actions[], unsigned count )
{
    mTestActions.clear();
    while( count > 0 ) {
        PathSearchAction* action = actions[--count];
        action->setEnabled( false ); // default to disabled until tested
        mTestActions.push_front( action );
    }

    testNextAction();
}

void PathFinderController::onResult( bool ok, PathSearchCriteria criteria )
{
    // filter any stale results
    if ( criteria == *mCurAction->getCriteria() ) {
        mCurAction->setEnabled( ok );

        // only test next if this result is unchanged given any remaining ones won't be valid if this one isn't
        testNextAction();
    }
}

void PathFinderController::onPath( PathSearchCriteria criteria, PieceListManager* path )
{
    // filter any stale results
    if ( criteria == *mCurAction->getCriteria() ) {
        emit pathFound( path, &(*mCurAction) );
    }
}

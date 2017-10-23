#include <iostream>
#include "pathfindercontroller.h"
#include "pathfinder.h"
#include "pathsearchaction.h"
#include "game.h"


PathFinderController::PathFinderController(QObject *parent) : QObject(parent), mCurCriteria(0), mCurAction(0)
{
}

void PathFinderController::init()
{
    mPathFinder.setParent(this);
    qRegisterMetaType<PathSearchCriteria>("PathSearchCriteria");
    QObject::connect( &mPathFinder, &PathFinder::testResult, this, &PathFinderController::onResult, Qt::QueuedConnection );
    if ( !((bool) QObject::connect( &mPathFinder, &PathFinder::pathFound,  this, &PathFinderController::onPath, Qt::QueuedConnection )) ) {
        std::cout << "*** connection failed" << std::endl;
    }
}

bool PathFinderController::doAction( PathSearchAction* action, bool testOnly )
{
    mCurAction = action;
    mCurCriteria = &action->getCriteria();
    return mPathFinder.execCriteria( mCurCriteria, testOnly );
}

void PathFinderController::testNextAction()
{
    if ( mTestActions.size() > 0 ) {
        mCurAction = mTestActions.front();
        mCurCriteria = &mCurAction->getCriteria();
        mTestActions.pop_front();
        doAction( mCurAction, true );
    } else {
        mCurAction = 0;
//        mCurCriteria = 0;
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

bool PathFinderController::testCriteria( PathSearchCriteria* criteria )
{
    mCurCriteria = criteria;
    return mPathFinder.execCriteria( mCurCriteria );
}

bool PathFinderController::buildTilePushPath( ModelVector target )
{
    return mPathFinder.buildTilePushPath( target );
}

void PathFinderController::onResult( bool ok, PathSearchCriteria criteria )
{
    // filter any stale results
    if ( mCurCriteria && criteria == *mCurCriteria ) {
        if ( mCurAction ) {
            mCurAction->setEnabled( ok );
        }
        emit testResult( ok, &criteria );

        // only test next if this result is unchanged given any remaining ones won't be valid if this one isn't
        testNextAction();
    }
}

void PathFinderController::onPath( PathSearchCriteria criteria, PieceListManager* path )
{
    // filter any stale results
    if ( mCurCriteria && criteria == *mCurCriteria ) {
        emit pathFound( path, &(*mCurCriteria) );
    } else {
        std::cout << "PathFinderController::onPath discarding stale path" << std::endl;
    }
}

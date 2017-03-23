#include <iostream>
#include "pathfindercontroller.h"
#include "pathfinder.h"
#include "pathsearchaction.h"
#include "game.h"


PathFinderController::PathFinderController(QObject *parent) : QObject(parent)
{
}

void PathFinderController::init( Game* game )
{
    setParent( game );
    mPathFinder.setParent(this);
    qRegisterMetaType<PathSearchCriteria>("PathSearchCriteria");
    QObject::connect( &mPathFinder, &PathFinder::testResult, this, &PathFinderController::onResult, Qt::QueuedConnection );
    QObject::connect( &mPathFinder, &PathFinder::pathFound,  this, &PathFinderController::onPath,   Qt::QueuedConnection );
}

void PathFinderController::doAction( std::shared_ptr<PathSearchAction> action, bool testOnly )
{
    mCurAction = action;
    mPathFinder.findPath( action->getCriteria(), testOnly );
}

void PathFinderController::testNextAction()
{
    if ( mTestActions.size() > 0 ) {
        mCurAction = mTestActions.front();
        mTestActions.pop_front();
        doAction( mCurAction, true );
    }
}

void PathFinderController::testActions( std::shared_ptr<PathSearchAction> actions[], unsigned count )
{
    mTestActions.clear();
    while( count > 0 ) {
        std::shared_ptr<PathSearchAction> action = actions[--count];
        action->setEnabled( false ); // default to disabled until tested
        mTestActions.push_front( action );
    }

    testNextAction();
}

void printCriteria( PathSearchCriteria criteria )
{
    std::cout << "(" << criteria.getStartCol() << "," << criteria.getStartRow() << ")/" << criteria.getStartDirection()
              << " (" << criteria.getTargetCol() << "," << criteria.getTargetRow()
              << ((criteria.getFocus()==TANK) ? ") TANK " : ") MOVE ") << criteria.getMoveWhenFound()  << std::endl;
}

void PathFinderController::onResult( bool ok, PathSearchCriteria criteria )
{
    std::cout << "result: "; printCriteria( criteria );
    std::cout << "action: "; printCriteria( *mCurAction->getCriteria() );

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

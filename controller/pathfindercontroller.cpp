#include <iostream>
#include "pathfindercontroller.h"
#include "pathfinder.h"
#include "pathsearchaction.h"
#include "game.h"
#include "model/tank.h"

PathFinderController::PathFinderController(QObject *parent) : QObject(parent)
{
}

void PathFinderController::init( Game* game )
{
    setParent( game );
    mPathFinder.setParent(this);
    QObject::connect( &mPathFinder, &PathFinder::testResult, this, &PathFinderController::onResult );
    QObject::connect( &mPathFinder, &PathFinder::pathFound,      this, &PathFinderController::onPath   );
}

void PathFinderController::doAction( std::shared_ptr<PathSearchAction> action, bool testOnly )
{
    Game* game = getGame(this);
    if ( game ) {
        Tank* tank = game->getTank();
        mStartCol = tank->getCol();
        mStartRow = tank->getRow();
        mStartDirection = tank->getRotation().toInt();

        mTargetCol = action->getTargetCol();
        mTargetRow = action->getTargetRow();
        mMoveWhenFound = action->getMoveWhenFound();

        mPathFinder.findPath( mTargetCol, mTargetRow, mStartCol, mStartRow, mStartDirection, testOnly );
    }
}

void PathFinderController::doAction( std::shared_ptr<PathSearchAction> action )
{
    doAction( action, false );
}

void PathFinderController::testNextAction()
{
    if ( mTestActions.size() > 0 ) {
        if ( std::shared_ptr<PathSearchAction> action = mTestActions.front().lock() ) {
            doAction( action, true );
        }
    }
}

void PathFinderController::testActions( std::shared_ptr<PathSearchAction> actions[], unsigned count )
{
    mTestActions.clear();
    while( count > 0 ) {
        std::shared_ptr<PathSearchAction> action = actions[--count];
        action->setEnabled( false ); // default to disabled until tested
        std::weak_ptr<PathSearchAction> wref = action;
        mTestActions.push_front( wref );
    }

    testNextAction();
}

void PathFinderController::onResult( bool ok, int targetCol, int targetRow, int startCol, int startRow, int startDirection )
{
    if ( targetCol == mTargetCol && targetRow == mTargetRow
      && startCol  == mStartCol  && startRow  == mStartRow && startDirection == mStartDirection
      && mTestActions.size() > 0 ) {
        if ( auto action = mTestActions.front().lock() ) {
            action->setEnabled( ok );

            mTestActions.pop_front();
            testNextAction();
        } else {
            std::cout << "*** weakref lost" << std::endl;
        }
    }
}

void PathFinderController::onPath( int targetCol, int targetRow, int startCol, int startRow, int targetRotation,
             PieceListManager* path )
{
    Game* game = getGame(this);

    if ( game ) {
        Tank* tank = game->getTank();

        // vet the original criteria to guard against a stale result
        if ( targetCol == mTargetCol && targetRow == mTargetRow
          && startCol == tank->getCol() && startRow == tank->getRow() && targetRotation == tank->getRotation() ) {
            emit pathFound( path );

            tank->getMoves()->reset( path );
            if ( mMoveWhenFound ) {
                tank->move();
            }
        }
    }
}

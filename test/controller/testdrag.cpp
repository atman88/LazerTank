#include <iostream>
#include "../testmain.h"
#include "dragactivity.h"
#include "gameregistry.h"
#include "game.h"
#include "movecontroller.h"
#include "pathfindercontroller.h"
#include "model/tank.h"

using namespace std;

void TestMain::testDragTank()
{
    initGame(
      "SSS\n"
      "S..\n"
      ".MT\n"
      "...\n" );

    DragActivity dragActivity( &mRegistry->getGame() );
    QCOMPARE( dragActivity.getState(), Inactive );
    QCOMPARE( dragActivity.getFocusPoint().isNull(), true );

    dragActivity.start( mRegistry->getTank().getPoint() );
    QCOMPARE( dragActivity.getState(), Selecting );
    QCOMPARE( dragActivity.getFocusPoint(), mRegistry->getTank().getPoint() );

    // test off-board:
    dragActivity.onDragTo( ModelPoint(3,2) );
    QCOMPARE( dragActivity.getState(), Forbidden );
    QCOMPARE( mRegistry->getMoveController().getMoves().size(), 0 );

    // test valid drag:
    dragActivity.onDragTo( ModelPoint(2,1) );
    QCOMPARE( dragActivity.getState(), Selecting );
    QCOMPARE( mRegistry->getMoveController().getMoves().size(), 1 );

    // test drag over stone:
    dragActivity.onDragTo( ModelPoint(2,0) );
    QCOMPARE( dragActivity.getState(), Forbidden );
    QCOMPARE( mRegistry->getMoveController().getMoves().size(), 1 );

    // test drag undo:
    dragActivity.onDragTo( mRegistry->getTank().getPoint() );
    QCOMPARE( dragActivity.getState(), Selecting );
    QCOMPARE( mRegistry->getMoveController().getMoves().size(), 0 );

    // test drag with initial rotate:
    dragActivity.onDragTo( ModelPoint(2,3) );
    QCOMPARE( dragActivity.getState(), Selecting );
    QCOMPARE( mRegistry->getMoveController().getMoves().size(), 2 );
    // undo it:
    dragActivity.onDragTo( mRegistry->getTank().getPoint() );
    QCOMPARE( dragActivity.getState(), Selecting );
    QCOMPARE( mRegistry->getMoveController().getMoves().size(), 0 );

    // test stop:
    dragActivity.stop();
    QCOMPARE( dragActivity.getState(), Inactive );
    QCOMPARE( dragActivity.getFocusPoint().isNull(), true );
}

void TestMain::testDragPoint()
{
    initGame(
      "SSS\n"
      "S..\n"
      ".MT\n"
      "...\n" );

    DragActivity dragActivity( &mRegistry->getGame() );

    // select forbidden (stone):
    dragActivity.start( ModelPoint(0,0) );
    QCOMPARE( dragActivity.getState(), Forbidden );
    dragActivity.stop();
    QCOMPARE( dragActivity.getState(), Inactive );

#ifdef NOTDEF
  // Want to use google mock for the path finder controller here
    // select
    qRegisterMetaType<DragState>("DragState");
    dragActivity.start( ModelPoint(2,3) );
    QSignalSpy dragSpy( &dragActivity, SIGNAL(stateChanged(DragState)) );
    cout << "dragSpy valid: " << dragSpy.isValid() << endl;
    if ( !dragSpy.wait(1000) ) {
        cout << "wait for Selecting state returned false" << endl;
    }
    cout << "stateChange count=" << dragSpy.size() << " state=" << dragActivity.getState() << endl;
    QCOMPARE( dragActivity.getState(), Selecting );
    QCOMPARE( mRegistry->getMoveController().getMoves().size(), 2 ); // rotation + move

    // undo
    dragActivity.onDragTo( mRegistry->getTank().getPoint() );
    QCOMPARE( mRegistry->getMoveController().getMoves().size(), 0 );
#endif // NOTDEF
}

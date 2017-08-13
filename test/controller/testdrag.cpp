#include <iostream>
#include "../testmain.h"
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

    MoveController& moveController = mRegistry.getMoveController();
    QCOMPARE( moveController.getDragState(), Inactive );

    moveController.dragStart( mRegistry.getTank().getPoint() );
    QCOMPARE( moveController.getDragState(), Selecting );
    QCOMPARE( moveController.getFocusVector(), mRegistry.getTank().getVector() );

    // test off-board:
    moveController.onDragTo( ModelPoint(3,2).toViewCenterSquare() );
    QCOMPARE( moveController.getDragState(), Forbidden );
    if ( ModelPoint* point = moveController.getMoves().getBack() ) {
        QVERIFY( mRegistry.getTank().getPoint().equals(*point) );
    }

    // test valid drag:
    moveController.onDragTo( ModelPoint(2,1).toViewCenterSquare() );
    QCOMPARE( moveController.getDragState(), Selecting );
    if ( ModelVector* vector = moveController.getMoves().getBack() ) {
        QVERIFY( ModelVector(2,1,0).equals( *vector ) );
    } else {
        QFAIL( "vaid drag has no moves" );
    }

    // test drag over stone:
    moveController.onDragTo( ModelPoint(2,0).toViewCenterSquare() );
    QCOMPARE( moveController.getDragState(), Forbidden );
    if ( ModelVector* vector = moveController.getMoves().getBack() ) {
        QVERIFY( ModelVector(2,1,0).equals( *vector ) );
    } else {
        QFAIL( "drag over stone has no moves" );
    }

    // test drag undo:
    moveController.onDragTo( mRegistry.getTank().getPoint().toViewCenterSquare() );
    QCOMPARE( moveController.getDragState(), Selecting );
    QVERIFY( moveController.getMoves().size() <= 1 );

    // test drag with initial rotate:
    moveController.onDragTo( ModelPoint(2,3).toViewCenterSquare() );
    QCOMPARE( moveController.getDragState(), Selecting );
    QCOMPARE( moveController.getMoves().size(), 2 );
    // undo it:
    moveController.onDragTo( mRegistry.getTank().getPoint().toViewCenterSquare() );
    QCOMPARE( moveController.getDragState(), Selecting );
    QVERIFY( moveController.getMoves().size() <= 1 );

    // test stop:
    moveController.dragStop();
    QCOMPARE( moveController.getDragState(), Inactive );
}

void TestMain::testDragWithMove()
{
    initGame(
      "...\n"
      "T..\n"
      "...\n" );

    MoveController& moveController = mRegistry.getMoveController();

    ModelVector v( mRegistry.getTank().getVector() );
    moveController.dragStart( v );

    // test direction change:
    v.mAngle = (v.mAngle + 90) % 360;
    moveController.move( v.mAngle );
    QCOMPARE( v, moveController.getFocusVector() );

    // attempt column change:
    moveController.move( v.mAngle );
    QCOMPARE( v, moveController.getFocusVector() );
}

void TestMain::testDragPoint()
{
    initGame(
      "SSS\n"
      "S..\n"
      ".MT\n"
      "...\n" );

    MoveController& moveController = mRegistry.getMoveController();

    // select forbidden (stone):
    moveController.dragStart( ModelPoint(0,0) );
    QCOMPARE( moveController.getDragState(), Forbidden );
    moveController.dragStop();
    QCOMPARE( moveController.getDragState(), Inactive );

#ifdef NOTDEF
  // Want to use google mock for the path finder controller here
    // select
    qRegisterMetaType<DragState>("DragState");
    moveController.start( ModelPoint(2,3) );
    QSignalSpy dragSpy( &moveController, SIGNAL(stateChanged(DragState)) );
    cout << "dragSpy valid: " << dragSpy.isValid() << endl;
    if ( !dragSpy.wait(1000) ) {
        cout << "wait for Selecting state returned false" << endl;
    }
    cout << "stateChange count=" << dragSpy.size() << " state=" << dragActivity.getState() << endl;
    QCOMPARE( moveController.getState(), Selecting );
    QCOMPARE( moveController.getMoves().size(), 2 ); // rotation + move

    // undo
    moveController.onDragTo( mRegistry.getTank().getPoint().toViewCenterSquare() );
    QCOMPARE( moveController.getMoves().size(), 0 );
#endif // NOTDEF
}

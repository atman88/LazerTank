#include <iostream>
#include "../testmain.h"
#include "gameregistry.h"
#include "game.h"
#include "movecontroller.h"
#include "pathfindercontroller.h"
#include "model/tank.h"
#include "../test/util/testasync.h"

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
    QCOMPARE( moveController.getDragState(), DraggingTank );
    QCOMPARE( moveController.getFocusVector(), mRegistry.getTank().getVector() );

    // test off-board:
    moveController.onDragTo( ModelPoint(3,2).toViewCenterSquare() );
    QCOMPARE( moveController.getDragState(), Forbidden );
    if ( ModelPoint* point = moveController.getMoves().getBack() ) {
        QVERIFY( mRegistry.getTank().getPoint().equals(*point) );
    }

    // test valid drag:
    moveController.onDragTo( ModelPoint(2,1).toViewCenterSquare() );
    QCOMPARE( moveController.getDragState(), DraggingTank );
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
    QCOMPARE( moveController.getDragState(), DraggingTank );
    QVERIFY( moveController.getMoves().size() <= 1 );

    // test drag with initial rotate:
    moveController.onDragTo( ModelPoint(2,3).toViewCenterSquare() );
    QCOMPARE( moveController.getDragState(), DraggingTank );
    QCOMPARE( moveController.getMoves().size(), 2 );
    // undo it:
    moveController.onDragTo( mRegistry.getTank().getPoint().toViewCenterSquare() );
    QCOMPARE( moveController.getDragState(), DraggingTank );
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

class TestPathFinderController : public PathFinderController, public TestAsync
{
public:
    TestPathFinderController() : PathFinderController(0), mError(0), mReceived(false)
    {
    }

    void init()
    {
        PathFinderController::init();
        QObject::connect( this, &PathFinderController::testResult, this, &TestPathFinderController::verifyTestResult );
    }

    bool condition()
    {
        return mReceived;
    }

    bool testBool( bool expression, const char* error )
    {
        if ( !expression && !mError ) {
            mError = error;
        }
        return expression;
    }

    bool testDrag( ModelPoint point, std::set<ModelPoint> expected )
    {
        mReceived = false;
        mExpected = expected;
        GameRegistry* registry = getRegistry(this);
        registry->getMoveController().dragStart( point );
        return testBool( registry->getMoveController().getDragState() == Searching, "dragState != Searching" )
            && testBool( test(), "timeout" );
    }

public slots:
    void verifyTestResult( bool ok, PathSearchCriteria* criteria )
    {
        if ( testBool( ok, "verify: received false" ) ) {
            testBool( mExpected.size() == criteria->getTileDragTestResult()->mPossibleApproaches.size(), "counts differ" );
            auto expectedIt = mExpected.cbegin();
            for( auto actualIt : criteria->getTileDragTestResult()->mPossibleApproaches ) {
                if ( testBool( actualIt.equals( *expectedIt++ ), "result differs" ) ) {
                    break;
                }
            }
        }
        mReceived = true;
    }

    std::set<ModelPoint> mExpected;
    const char* mError;
    bool mReceived;
};

void TestMain::testDragTile()
{
    TestPathFinderController* pathFinderController = new TestPathFinderController();
    mRegistry.injectPathFinderController( pathFinderController );
    initGame(
      "T . ..\n"
      ".[/M..\n"
      ". . ..\n"
      ". M ..\n" );
    pathFinderController->init();
    qRegisterMetaType<PathSearchCriteria*>("PathSearchCriteria*");

    pathFinderController->testDrag( ModelPoint(1,1), { ModelPoint(1,2), ModelPoint(2,1) } );
    if ( pathFinderController->mError ) {
        QFAIL( pathFinderController->mError );
    }

    pathFinderController->testDrag( ModelPoint(1,3), { ModelPoint(0,3), ModelPoint(2,3), ModelPoint(2,3) } );
    if ( pathFinderController->mError ) {
        QFAIL( pathFinderController->mError );
    }
}

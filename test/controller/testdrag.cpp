#include <iostream>
#include "../testmain.h"
#include "gameregistry.h"
#include "game.h"
#include "movecontroller.h"
#include "pathfindercontroller.h"
#include "model/tank.h"
#include "util/workerthread.h"
#include "../test/util/testasync.h"

using namespace std;

#define TESTBOOL(expr,msg) testBool(expr,msg,__FILE__, __LINE__)

class TestMoveController : public MoveController
{
public:
    TestMoveController() : MoveController(0)
    {
    }

    PieceListManager& getDragMoves()
    {
        return mDragMoves;
    }
};

void TestMain::testDragTank()
{
    TestMoveController* moveController = new TestMoveController();
    mRegistry.injectMoveController( moveController );
    initGame(
      "SSS\n"
      "S..\n"
      ".MT\n"
      "...\n" );

    QCOMPARE( moveController->getDragState(), Inactive );

    moveController->dragStart( mRegistry.getTank().getPoint() );
    QCOMPARE( moveController->getDragState(), DraggingTank );
    QCOMPARE( moveController->getBaseFocusVector(), mRegistry.getTank().getVector() );

    // test off-board:
    moveController->onDragTo( ModelPoint(3,2).toViewCenterSquare() );
    QCOMPARE( moveController->getDragState(), ForbiddenTank );
    if ( ModelPoint* point = moveController->getDragMoves().getBack() ) {
        QVERIFY( mRegistry.getTank().getPoint().equals(*point) );
    }

    // test valid drag:
    moveController->onDragTo( ModelPoint(2,1).toViewCenterSquare() );
    QCOMPARE( moveController->getDragState(), DraggingTank );
    if ( ModelVector* vector = moveController->getDragMoves().getBack() ) {
        QVERIFY( ModelVector(2,1,0).equals( *vector ) );
    } else {
        QFAIL( "drag has no moves" );
    }

    // test drag over stone:
    moveController->onDragTo( ModelPoint(2,0).toViewCenterSquare() );
    QCOMPARE( moveController->getDragState(), ForbiddenTank );
    if ( ModelVector* vector = moveController->getDragMoves().getBack() ) {
        QVERIFY( ModelVector(2,1,0).equals( *vector ) );
    } else {
        QFAIL( "drag over stone has no moves" );
    }

    // test drag undo:
    moveController->onDragTo( mRegistry.getTank().getPoint().toViewCenterSquare() );
    QCOMPARE( moveController->getDragState(), DraggingTank );
    QVERIFY( moveController->getDragMoves().size() <= 1 );

    // test drag with initial rotate:
    moveController->onDragTo( ModelPoint(2,3).toViewCenterSquare() );
    QCOMPARE( moveController->getDragState(), DraggingTank );
    QCOMPARE( moveController->getDragMoves().size(), 2 );
    // undo it:
    moveController->onDragTo( mRegistry.getTank().getPoint().toViewCenterSquare() );
    QCOMPARE( moveController->getDragState(), DraggingTank );
    QVERIFY( moveController->getDragMoves().size() <= 1 );

    // test stop:
    moveController->dragStop();
    QCOMPARE( moveController->getDragState(), Inactive );
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
    QCOMPARE( v, moveController.getBaseFocusVector() );

    // attempt column change:
    moveController.move( v.mAngle );
    QCOMPARE( v, moveController.getBaseFocusVector() );
}

class TestPathFinderController : public PathFinderController, public TestAsync
{
public:
    TestPathFinderController() : PathFinderController(0), mExpectedPathCount(0), mError(0), mReceived(false)
    {
    }

    void init()
    {
        QObject::connect( this, &PathFinderController::testResult, this, &TestPathFinderController::verifyStartResult );
        QObject::connect( this, &PathFinderController::pathFound, this, &TestPathFinderController::verifyDragToResult );
    }

    bool condition()
    {
        return mReceived;
    }

    bool testBool( bool expression, const char* error, const char* file, int line )
    {
        if ( !expression && !mError ) {
            mError = error;
            QTest::qFail( error, file, line );
        }
        return expression;
    }

    // test starting a drag of a tile
    bool testStart( ModelPoint point, std::set<ModelPoint> expected )
    {
        cout << "dragStart (" << point.mCol << "," << point.mRow << ")" << endl;
        mReceived = false;
        mExpected = expected;
        GameRegistry* registry = getRegistry(this);
        registry->getMoveController().dragStart( point );
        return TESTBOOL( registry->getMoveController().getDragState() == Searching, "dragState != Searching" )
            && TESTBOOL( test(), "timeout" );
    }

    // test start a drag at an empty point
    bool testStart( ModelPoint point, int expectedCount )
    {
        cout << "dragStart " << expectedCount << endl;
        mExpectedPathCount = expectedCount;
        mReceived = false;
        GameRegistry* registry = getRegistry(this);
        registry->getMoveController().dragStart( point );
        return TESTBOOL( registry->getMoveController().getDragState() == Searching, "dragState != Searching" )
            && TESTBOOL( test(), "timeout" );
    }

    bool testDragTo( QPoint coord, int expectedCount )
    {
        cout << "onDragTo (" << coord.x() << "," << coord.y() << ")" << endl;
        mExpectedPathCount = expectedCount;
        mReceived = false;
        GameRegistry* registry = getRegistry(this);
        registry->getMoveController().onDragTo( coord );
        return TESTBOOL( registry->getMoveController().getDragState() == Searching, "dragState != Searching" )
            && TESTBOOL( test(), "timeout" );
    }

public slots:
    void verifyStartResult( bool ok, PathSearchCriteria* criteria )
    {
        cout << "verifyStartResult " << ok << endl;
        if ( TESTBOOL( ok, "verify: received false" ) ) {
            TESTBOOL( mExpected.size() == criteria->getTileDragTestResult()->mPossibleApproaches.size(), "counts differ" );
            auto expectedIt = mExpected.cbegin();
            for( auto actualIt : criteria->getTileDragTestResult()->mPossibleApproaches ) {
                if ( TESTBOOL( actualIt.equals( *expectedIt++ ), "result differs" ) ) {
                    break;
                }
            }
        }
        mReceived = true;
    }

    void verifyDragToResult( PieceListManager* path, PathSearchCriteria* /*criteria*/ )
    {
        cout << "verifyDragToResult size=" << path->size() << endl;
        TESTBOOL( path->size() == mExpectedPathCount, "unexpected path size" );
        mReceived = true;
    }

    std::set<ModelPoint> mExpected;
    int mExpectedPathCount;
    const char* mError;
    bool mReceived;
};

TestPathFinderController* TestMain::setupTestDrag( const char* map )
{
    TestPathFinderController* pathFinderController = new TestPathFinderController();
    mRegistry.injectPathFinderController( pathFinderController );
    initGame( map );
    pathFinderController->init();
    return pathFinderController;
}

void TestMain::testDragPoint()
{
    TestMoveController* moveController = new TestMoveController();
    mRegistry.injectMoveController( moveController );
    TestPathFinderController* pathFinderController = setupTestDrag(
      "S..\n"
      ".M.\n"
      "...\n"
      ".T.\n" );

    // select forbidden (stone):
    moveController->dragStart( ModelPoint(0,0) );
    QCOMPARE( moveController->getDragState(), Forbidden );
    moveController->dragStop();
    QCOMPARE( moveController->getDragState(), Inactive );

    pathFinderController->testStart( ModelPoint(1,2), 1 );

    // push the tile
    QPoint coord( ModelPoint(1,1).toViewCenterSquare() );
    moveController->onDragTo( coord );
    QCOMPARE( moveController->getDragMoves().size(), 2 );

    // cause the rotation change
    coord.setY( coord.y()+24/3 );
    moveController->onDragTo( coord );

    // undo the push
    moveController->onDragTo( ModelPoint(1,2).toViewCenterSquare() );
    QCOMPARE( moveController->getDragMoves().size(), 1 );
}

void TestMain::testDragTile()
{
    TestPathFinderController* pathFinderController = setupTestDrag(
      "T . ..\n"
      ".[/M..\n"
      ". . ..\n"
      ". M ..\n" );

    pathFinderController->testStart( ModelPoint(1,1), { ModelPoint(1,2), ModelPoint(2,1) } );
    if ( pathFinderController->mError ) {
        QFAIL( pathFinderController->mError );
    }

    pathFinderController->testStart( ModelPoint(1,3), { ModelPoint(0,3), ModelPoint(2,3), ModelPoint(2,3) } );
    if ( pathFinderController->mError ) {
        QFAIL( pathFinderController->mError );
    }
}

void TestMain::testFutureSelect()
{
    cout << "start testFutureSelect()" << endl;
    TestPathFinderController* pathFinderController = setupTestDrag(
      "..T\n"
      ".MM\n"
      "...\n" );

    // push the right-side tile
    pathFinderController->testStart( ModelPoint(2,1), { ModelPoint(2,0), ModelPoint(2,3) } );
    QPoint coord( ModelPoint(2,1).toViewCenterSquare() );
    coord.setY( coord.y()+24/3 );
    pathFinderController->testDragTo( coord, 1 );
    mRegistry.getMoveController().dragStop();

    // check the left-side possible angles
    pathFinderController->testStart( ModelPoint(1,1), { ModelPoint(1,0), ModelPoint(0,1), ModelPoint(2,1), ModelPoint(1,2) } );
}

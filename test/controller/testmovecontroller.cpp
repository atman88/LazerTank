#include <iostream>
#include <QObject>
#include <QRect>
#include "../testmain.h"
#include "../util/piecelistmanagerobserver.h"
#include "movecontroller.h"
#include "game.h"
#include "animationstateaggregator.h"
#include "model/tank.h"
#include "model/shotmodel.h"
#include "util/recorder.h"

using namespace std;


void setupTestMultiShot( TestMain* main, PieceListManagerObserver **shotObserver )
{
    QTextStream map(
      "TM...\n"
      ".....\n" );
    main->initGame( map );

    MoveController& moveController = main->getRegistry()->getMoveController();
    *shotObserver = new PieceListManagerObserver( moveController.getMoves(), 3, 3 );
    moveController.move( 90 );
    moveController.fire( 3 );
}

void completeTestMultiShot( TestMain* main, PieceListManagerObserver *shotObserver )
{
    MoveController& moveController = main->getRegistry()->getMoveController();

    moveController.move( 180 );
    QSignalSpy idleSpy( &moveController, &MoveController::idle );
    QVERIFY( idleSpy.wait( 4000 ) );

    shotObserver->printCounts();

    QVERIFY( main->getRegistry()->getGame().getBoard()->getPieceManager().typeAt(ModelPoint(4,0)) == TILE );
}

void TestMain::testMultiShotQueued()
{
    PieceListManagerObserver* shotObserver;
    setupTestMultiShot( this, &shotObserver );
    completeTestMultiShot( this, shotObserver );
}

void TestMain::testMultiShotShotDirty()
{
    PieceListManagerObserver* shotObserver;
    setupTestMultiShot( this, &shotObserver );

    DirtySpy dirtySpy( &mRegistry.getTank().getShot() );
    QVERIFY( dirtySpy.wait( 3000 ) );

    completeTestMultiShot( this, shotObserver );
}

void TestMain::testMultiShotShooterRelease()
{
    PieceListManagerObserver* shotObserver;
    setupTestMultiShot( this, &shotObserver );

    QSignalSpy releaseSpy( &mRegistry.getTank().getShot(), SIGNAL(shooterReleased()) );
    QVERIFY( releaseSpy.wait( 3000 ) );

    completeTestMultiShot( this, shotObserver );
}

void TestMain::testMultiShotShotFinished()
{
    PieceListManagerObserver* shotObserver;
    setupTestMultiShot( this, &shotObserver );

    QSignalSpy finishedSpy( &mRegistry.getShotAggregate(), &AnimationStateAggregator::finished );
    QVERIFY( finishedSpy.wait( 3000 ) );

    completeTestMultiShot( this, shotObserver );
}

void TestMain::testReplay()
{
    QTextStream map(
      "T.M.\n"
      ".M..\n"
      "....\n" );
    initGame( map );

    MoveController& moveController = mRegistry.getMoveController();
    Tank& tank = mRegistry.getTank();
    PieceSetManager& boardPieces = mRegistry.getGame().getBoard()->getPieceManager();

    QSignalSpy idleSpy( &moveController, &MoveController::idle );
    ModelVector pos = tank.getVector();

    moveController.move( 90 );
    QVERIFY( idleSpy.wait( 1000 ) );
    pos.mAngle = 90;
    QVERIFY( tank.getVector().equals(pos) );

    moveController.move( 90 );
    moveController.fire();
    QVERIFY( idleSpy.wait( 1000 ) );
    pos.mCol += 1;
    QVERIFY( tank.getVector().equals(pos) );
    QVERIFY( boardPieces.typeAt(ModelPoint(3,0)) == TILE );

    moveController.move( 180 );
    moveController.fire();
    QVERIFY( idleSpy.wait( 1000 ) );
    pos.mAngle = 180;
    QVERIFY( tank.getVector().equals(pos) );\

    if ( mRegistry.getShotAggregate().active() ) {
        QSignalSpy shotSpy( &mRegistry.getShotAggregate(), &AnimationStateAggregator::finished );
        shotSpy.wait( 1000 );
    }
    QVERIFY( boardPieces.typeAt(ModelPoint(1,2)) == TILE );

    std::cout << "testmovecontroller: start replay #moves=" << tank.getRecorder().getCount() << std::endl;

    mRegistry.getGame().replayLevel();
    QVERIFY( boardPieces.typeAt(ModelPoint(3,0)) == NONE );
    QVERIFY( boardPieces.typeAt(ModelPoint(1,2)) == NONE );

    QSignalSpy replaySpy( &moveController, &MoveController::replayFinished );
    QVERIFY( replaySpy.wait( 4000 ) );
    QVERIFY( boardPieces.typeAt(ModelPoint(3,0)) == TILE );

    if ( mRegistry.getShotAggregate().active() ) {
        QSignalSpy shotSpy( &mRegistry.getShotAggregate(), &AnimationStateAggregator::finished );
        shotSpy.wait( 1000 );
    }
    QVERIFY( boardPieces.typeAt(ModelPoint(1,2)) == TILE );
}

void TestMain::testMoveFocus()
{
    QTextStream map(
      "[T>..\n"
      " . ..\n" );
    initGame( map );

    MoveController& moveController = mRegistry.getMoveController();
    Tank& tank = mRegistry.getTank();
    moveController.move(90, false);
    QVERIFY( !tank.getVector().equals( moveController.getFocusVector() ) );
    moveController.setFocus( TANK );
    QCOMPARE( moveController.getFocus(), TANK );
    QCOMPARE( tank.getVector(), moveController.getFocusVector() );
    QVERIFY2( moveController.getMoves().size()==2, "Expected 2 moves: focus inject at (0,0), (1,0) " );

    moveController.setFocus( MOVE );
    QVERIFY( !tank.getVector().equals( moveController.getFocusVector() ) );
}

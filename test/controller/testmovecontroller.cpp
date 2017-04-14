#include <iostream>
#include <QSignalSpy>
#include "../testmain.h"

#include "movecontroller.h"
#include "game.h"

using namespace std;

void TestMain::testReplay()
{
    QTextStream map(
      "T.M.\n"
      ".M..\n"
      "....\n" );
    initGame( map );

    MoveController& moveController = mRegistry->getMoveController();
    Tank& tank = mRegistry->getTank();
    PieceSetManager* boardPieces = mRegistry->getGame().getBoard()->getPieceManager();

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
    QVERIFY( boardPieces->typeAt(3,0) == TILE );

    moveController.move( 180 );
    moveController.fire();
    QVERIFY( idleSpy.wait( 1000 ) );
    pos.mAngle = 180;
    QVERIFY( tank.getVector().equals(pos) );\

    if ( mRegistry->getShotAggregate().active() ) {
        QSignalSpy shotSpy( &mRegistry->getShotAggregate(), &AnimationStateAggregator::finished );
        shotSpy.wait( 1000 );
    }
    QVERIFY( boardPieces->typeAt(1,2) == TILE );

    std::cout << "testmovecontroller: start replay" << std::endl;

    mRegistry->getGame().replayLevel();
    QVERIFY( boardPieces->typeAt(3,0) == NONE );
    QVERIFY( boardPieces->typeAt(1,2) == NONE );

    QSignalSpy replaySpy( &moveController, &MoveController::replayFinished );
    QVERIFY( replaySpy.wait( 4000 ) );
    QVERIFY( boardPieces->typeAt(3,0) == TILE );

    if ( mRegistry->getShotAggregate().active() ) {
        QSignalSpy shotSpy( &mRegistry->getShotAggregate(), &AnimationStateAggregator::finished );
        shotSpy.wait( 1000 );
    }
    QVERIFY( boardPieces->typeAt(1,2) == TILE );
}

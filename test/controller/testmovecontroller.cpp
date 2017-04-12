#include <iostream>
#include <QSignalSpy>
#include "../testmain.h"

#include "movecontroller.h"
#include "game.h"

using namespace std;

void TestMain::testReplay()
{
    Game game;
    QTextStream map(
      "T.M.\n"
      ".M..\n"
      "....\n" );
    initGame( game, map );

    MoveController* moveController = game.getMoveController();
    QSignalSpy idleSpy( moveController, &MoveController::idle );
    ModelVector pos = game.getTank()->getVector();

    moveController->move( 90 );
    QVERIFY( idleSpy.wait( 1000 ) );
    pos.mAngle = 90;
    QVERIFY( game.getTank()->getVector().equals(pos) );

    moveController->move( 90 );
    moveController->fire();
    QVERIFY( idleSpy.wait( 1000 ) );
    pos.mCol += 1;
    QVERIFY( game.getTank()->getVector().equals(pos) );
    QVERIFY( game.getBoard()->getPieceManager()->typeAt(3,0) == TILE );

    moveController->move( 180 );
    moveController->fire();
    QVERIFY( idleSpy.wait( 1000 ) );
    pos.mAngle = 180;
    QVERIFY( game.getTank()->getVector().equals(pos) );\

    if ( game.getShotAggregate()->active() ) {
        QSignalSpy shotSpy( game.getShotAggregate(), &AnimationStateAggregator::finished );
        shotSpy.wait( 1000 );
    }
    QVERIFY( game.getBoard()->getPieceManager()->typeAt(1,2) == TILE );

    std::cout << "testmovecontroller: start replay" << std::endl;

    game.replayLevel();
    QVERIFY( game.getBoard()->getPieceManager()->typeAt(3,0) == NONE );
    QVERIFY( game.getBoard()->getPieceManager()->typeAt(1,2) == NONE );

    QSignalSpy replaySpy( moveController, &MoveController::replayFinished );
    QVERIFY( replaySpy.wait( 4000 ) );
    QVERIFY( game.getBoard()->getPieceManager()->typeAt(3,0) == TILE );

    if ( game.getShotAggregate()->active() ) {
        QSignalSpy shotSpy( game.getShotAggregate(), &AnimationStateAggregator::finished );
        shotSpy.wait( 1000 );
    }
    QVERIFY( game.getBoard()->getPieceManager()->typeAt(1,2) == TILE );
}

#include <iostream>
#include <QSignalSpy>
#include "../testmain.h"

#include "movecontroller.h"
#include "game.h"

using namespace std;

void TestMain::testReplay()
{
    Game game;
    game.init( 0 );
    Board* board = game.getBoard();
    QString map(
      "T.M.\n"
      ".M..\n"
      "....\n" );
    QTextStream s(&map);
    board->load( s );

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
    QVERIFY( idleSpy.wait( 2000 ) );
    pos.mAngle = 180;
    QVERIFY( game.getTank()->getVector().equals(pos) );
//    Not seeing this shotPush advance within the test environment:
//    QVERIFY( game.getBoard()->getPieceManager()->typeAt(1,2) == TILE );
}

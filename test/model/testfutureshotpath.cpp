#include "../testmain.h"
#include "model/futureshotpath.h"
#include "controller/game.h"
#include "controller/movecontroller.h"
#include "model/board.h"
#include "model/tank.h"

void TestMain::testFutureShotPath()
{
    initGame(
      "[M/W[\\M\n"
      " w . <\n"
      " M . .\n"
      " T m .\n" );

    FutureShotPathManager manager;
    manager.setParent( mRegistry );

    MovePiece move( MOVE, mRegistry->getTank().getCol(), mRegistry->getTank().getRow(), 0, 6 );
    const FutureShotPath* path = manager.updatePath(&move);
    QCOMPARE( path->getUID(), move.getShotPathUID() );

    Game& game = mRegistry->getGame();
    Board* futureBoard = game.getBoard(true);

    // verify that this is the future board:
    QCOMPARE(game.isMasterBoard(futureBoard), false);

    QCOMPARE( futureBoard->getPieceManager().typeAt( ModelPoint(2,3) ), CANNON );

    move.setShotCount( 5 );
    manager.updatePath(&move);
    QCOMPARE( futureBoard->getPieceManager().typeAt( ModelPoint(2,3) ), CANNON );

    move.setShotCount( 4 );
    manager.updatePath(&move);
    QCOMPARE( futureBoard->getPieceManager().typeAt( ModelPoint(2,3) ), NONE );

    move.setShotCount( 0 );
    manager.updatePath(&move);
    QCOMPARE( (int) game.getDeltaPieces()->size(), 0 );
}

/**
 * @brief Verify that the master tank point doesn't cause a false positive hit detection
 */
void TestMain::testFutureShotThruMasterTank()
{
    initGame(
      "W\n"
      "T\n"
      ".\n" );
    MoveController& moveController = mRegistry->getMoveController();
    moveController.move(180); // rotate down
    moveController.move(180); // move down -> 0,2
    moveController.move(  0); // rotate up
    moveController.fire();
    QCOMPARE( mRegistry->getGame().getBoard(true)->tileAt( ModelPoint(0,0) ), WOOD_DAMAGED );
}

void TestMain::testFutureMultiShotThruTank()
{
    QTextStream map(
      ". . .\n"
      ". M .\n"
      ".[T<.\n"
      ". . .\n" );
    initGame( map );

    MoveController& moveController = mRegistry->getMoveController();
    moveController.move(270);
    moveController.move(0);
    moveController.move(0);
    moveController.move(0);
    moveController.move(90);
    moveController.move(90);
    moveController.move(180);
    moveController.fire(2);
    QCOMPARE( TILE, mRegistry->getGame().getBoard(true)->getPieceManager().pieceAt(ModelPoint(1,3))->getType() );
}

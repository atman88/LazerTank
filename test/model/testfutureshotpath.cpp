#include "../testmain.h"
#include "model/futureshotpath.h"
#include "controller/game.h"
#include "model/board.h"

void TestMain::testFutureShotPath()
{
    Game game;
    game.init( 0 );

    Board* masterBoard = game.getBoard();
    QString map(
      "[M/W[\\M\n"
      " w . <\n"
      " M . .\n"
      " T m .\n" );
    QTextStream s(&map);
    masterBoard->load( s );

    FutureShotPathManager manager;
    manager.setParent( &game );

    MovePiece move( MOVE, game.getTank()->getCol(), game.getTank()->getRow(), 0, 6 );
    const FutureShotPath* path = manager.updatePath(&move);
    QCOMPARE( path->getUID(), move.getShotPathUID() );

    Board* futureBoard = game.getBoard(true);

    // verify that this is the future board:
    QCOMPARE(game.isMasterBoard(futureBoard), false);

    QCOMPARE( futureBoard->getPieceManager()->typeAt( 2, 3 ), CANNON );

    move.setShotCount( 5 );
    manager.updatePath(&move);
    QCOMPARE( futureBoard->getPieceManager()->typeAt( 2, 3 ), CANNON );

    move.setShotCount( 4 );
    manager.updatePath(&move);
    QCOMPARE( futureBoard->getPieceManager()->typeAt( 2, 3 ), NONE );

    move.setShotCount( 0 );
    manager.updatePath(&move);
    QCOMPARE( game.getDeltaPieces()->size(), 0UL );
}

/**
 * @brief Verify that the master tank point doesn't cause a false positive hit detection
 */
void TestMain::testFutureShotThruMasterTank()
{
    Game game;
    game.init( 0 );

    QString map =
      "W\n"
      "T\n"
      ".\n";
    QTextStream s(&map);
    game.getBoard()->load( s );
    game.getMoveController()->move(180); // rotate down
    game.getMoveController()->move(180); // move down -> 0,2
    game.getMoveController()->move(  0); // rotate up
    game.getMoveController()->fire();
    QCOMPARE( game.getBoard(true)->tileAt( 0, 0 ), WOOD_DAMAGED );
}

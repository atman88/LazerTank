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

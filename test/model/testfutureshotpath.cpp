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

    int shotCount = 7;
    MovePiece move( MOVE, game.getTank()->getCol(), game.getTank()->getRow(), 0, shotCount );
    const FutureShotPath* path = manager.addPath(&move);
    QCOMPARE( path->getUID(), move.getShotPathUID() );

    Board* futureBoard = game.getBoard(true);

    // verify that this is the future board:
    QCOMPARE(game.isMasterBoard(futureBoard), false);
}

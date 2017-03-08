#include <iostream>
#include "../testmain.h"
#include "controller/game.h"

#include "view/boardwindow.h"

using namespace std;

void TestMain::testMove()
{
    Game game;
    BoardWindow window;
    game.init( &window );

    Board* board = game.getBoard();
    QString map(
      "T..F......\n"
      "..........\n"
      ".M........\n"
      "..........\n"
      "..........\n" );
    QTextStream s(&map);
    board->load( s );
    cout << "board " << board->getWidth() << "x" << board->getHeight() << std::endl;

    const PieceSet* tiles = board->getPieceManager()->getPieces();
    QCOMPARE( tiles->size(), 1UL );
    QCOMPARE( (*tiles->begin())->encodedPos(), Piece::encodePos(1,2));

    // check off-board values;
    QCOMPARE( game.canPlaceAtNonFuturistic(TANK,-1, 0,270), false );
    QCOMPARE( game.canPlaceAtNonFuturistic(TANK, 0,-1,  0), false );

    cout << "tank " << board->getTankStartCol() << "," << board->getTankStartRow() << std::endl;
    QCOMPARE( game.canPlaceAtNonFuturistic(TANK,board->getTankStartCol(),board->getTankStartRow(),0), true );
}

void testCannonAt( int tankCol, int tankRow, /*int expectedCol, int expectedRow,*/ Game* game )
{
    cout << "testCannonAt " << tankCol << "," << tankRow << endl;
    game->getTank()->reset(tankCol,tankRow);
    game->onTankMoved(tankCol,tankRow);
    QCOMPARE( game->getCannonShot().getLeadingCol(), tankCol );
    QCOMPARE( game->getCannonShot().getLeadingRow(), tankRow );
}

void TestMain::testCannon()
{
    Game game;
    BoardWindow window;
    game.init( &window );
    QString map(
    "v...<\n"
    ".....\n"
    "..T..\n"
    ".....\n"
    ">...^\n" );
    QTextStream s(&map);
    game.getBoard()->load( s );

    testCannonAt( 2, 0, &game );
    testCannonAt( 4, 2, &game );
    testCannonAt( 2, 4, &game );
    testCannonAt( 0, 2, &game );
}

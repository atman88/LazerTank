#include <iostream>
#include "../testmain.h"
#include "controller/game.h"

using namespace std;

void TestMain::testMove()
{
    Game game;
    BoardWindow window;

    game.init( &window );

    QString fileName( ":/maps/testsimple.txt" );
    Board* board = game.getBoard();
    board->load( fileName );
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

void TestMain::testCannon()
{
    Game game;
    BoardWindow window;
    game.init( &window );
    QString fileName( ":/maps/testcannon.txt" );
    game.getBoard()->load( fileName );

    Tank* tank = game.getTank();
    tank->move(0);
    tank->move(0);
}

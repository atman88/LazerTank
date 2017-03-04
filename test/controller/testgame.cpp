#include <iostream>
#include "../testmain.h"
#include "controller/Game.h"

void TestMain::testMove()
{
    Board board;
    QString fileName( ":/maps/testsimple.txt" );
    board.load( fileName );
    cout << "board " << board.getWidth() << "x" << board.getHeight() << std::endl;

    const PieceSet* tiles = board.getPieceManager()->getPieces();
    QCOMPARE( tiles->size(), 1UL );
    QCOMPARE( (*tiles->begin())->encodedPos(), Piece::encodePos(1,2));

    Game game( &board );
    // check off-board values;
    QCOMPARE( game.canPlaceAtNonFuturistic(TANK,-1, 0,270), false );
    QCOMPARE( game.canPlaceAtNonFuturistic(TANK, 0,-1,  0), false );

    cout << "tank " << board.mInitialTankX << "," << board.mInitialTankY << std::endl;
    QCOMPARE( game.canPlaceAtNonFuturistic(TANK,board.mInitialTankX,board.mInitialTankY,0), true );
}

void TestMain::testCannon()
{
    BoardWindow window;
    Board board;
    Game game( &board );
    game.init( &window );
    QString fileName( ":/maps/testcannon.txt" );
    board.load( fileName );

    Tank* tank = window.getTank();
    tank->move(0);
    tank->move(0);
}

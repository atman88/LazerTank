#include <iostream>
#include "../testmain.h"
#include "controller/game.h"

#include "model/shotmodel.h"
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
    cout << "board " << board->getWidth() << "x" << board->getHeight() << endl;

    const PieceSet* tiles = board->getPieceManager()->getPieces();
    QCOMPARE( tiles->size(), 1UL );
    QCOMPARE( (*tiles->begin())->encodedPos(), Piece::encodePos(1,2));

    // check off-board values;
    QCOMPARE( game.canPlaceAt(TANK,-1, 0,270), false );
    QCOMPARE( game.canPlaceAt(TANK, 0,-1,  0), false );

    cout << "tank " << board->getTankStartCol() << "," << board->getTankStartRow() << endl;
    QCOMPARE( game.canPlaceAt(TANK,board->getTankStartCol(),board->getTankStartRow(),0), true );
}

void testCannonAt( int tankCol, int tankRow, Game* game )
{
    cout << "testCannonAt " << tankCol << "," << tankRow << endl;
    game->getTank()->reset(tankCol,tankRow);
    game->getCannonShot().reset();

    game->sightCannons();

    SignalReceptor killedReceptor;
    ShotModel& shot = game->getCannonShot();
    QObject::connect( &shot, &ShotModel::tankKilled, &killedReceptor, &SignalReceptor::receive );
    for( int seq = shot.getSequence().toInt(); seq < 5 && !killedReceptor.mReceived; ++seq ) {
        shot.setSequence( QVariant(seq) );
    }
    QObject::disconnect( &shot, 0, &killedReceptor, 0 );

    QCOMPARE( killedReceptor.mReceived, true );
}

void TestMain::testCannon()
{
    Game game;
    game.init( 0 );
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

void testFuturePushToward( int direction, Game* game )
{
    cout << "testFuturePushToward " << direction << endl;

    Tank* tank = game->getTank();
    tank->getMoves()->reset();

    // move twice; first may merel rotate the tank but more importantly primes the move list:
    tank->move( direction );
    tank->move( direction );
    QVERIFY( game->getDeltaPieces()->size() > 0 );
    game->undoLastMove();
    QCOMPARE( game->getDeltaPieces()->size(), 0UL );
}

void TestMain::testPush()
{
    Game game;
    game.init( 0 );
    QString map(
        ".. m ..\n"
        ".. M ..\n"
        ".. . ..\n"
        "w< T >.\n"
        "..[M/..\n"
        ".. w ..\n" );
    QTextStream s(&map);
    game.getBoard()->load( s );

    // force the move aggregate active so the tank won't be woken up:
    game.getMoveAggregate()->onStateChanged( QAbstractAnimation::Running, QAbstractAnimation::Stopped );

    testFuturePushToward(  90, &game );
    testFuturePushToward( 180, &game );
    testFuturePushToward( 270, &game );
    testFuturePushToward(   0, &game );
}

#include <iostream>
#include "../testmain.h"
#include "controller/game.h"

#include "model/shotmodel.h"

using namespace std;

void TestMain::testGameMove()
{
    Game game;
    initGame( game,
      "T..F......\n"
      "..........\n"
      ".M........\n"
      "..........\n"
      "..........\n" );
    Board* board = game.getBoard();
    cout << "board " << board->getWidth() << "x" << board->getHeight() << endl;

    const PieceSet* tiles = board->getPieceManager()->getPieces();
    QCOMPARE( tiles->size(), 1UL );
    QCOMPARE( (*tiles->begin())->encodedPos(), Piece::encodePos(1,2));

    // check off-board values;
    QCOMPARE( game.canPlaceAt(TANK,ModelPoint(-1, 0),270), false );
    QCOMPARE( game.canPlaceAt(TANK,ModelPoint( 0,-1),  0), false );

    QCOMPARE( game.canPlaceAt( TANK, board->getTankStartPoint(), 0 ), true );
}

void testCannonAt( int tankCol, int tankRow, Game* game )
{
    cout << "testCannonAt " << tankCol << "," << tankRow << endl;
    game->getTank()->onBoardLoaded( ModelPoint(tankCol,tankRow) );
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

void TestMain::testGameCannon()
{
    Game game;
    initGame( game,
      "v...<\n"
      ".....\n"
      "..T..\n"
      ".....\n"
      ">...^\n" );

    testCannonAt( 2, 0, &game );
    testCannonAt( 4, 2, &game );
    testCannonAt( 2, 4, &game );
    testCannonAt( 0, 2, &game );
}

void testFuturePushToward( int direction, Game* game )
{
    cout << "testFuturePushToward " << direction << endl;

    MoveController* moveController = game->getMoveController();
    moveController->getMoves()->reset();

    // move twice; first may merely rotate the tank but more importantly primes the move list:
    moveController->move( direction );
    moveController->move( direction );
    QVERIFY( game->getDeltaPieces()->size() > 0 );
    game->undoLastMove();
    QCOMPARE( game->getDeltaPieces()->size(), 0UL );
}

void TestMain::testGamePush()
{
    Game game;
    initGame( game,
        ".. m ..\n"
        ".. M ..\n"
        ".. . ..\n"
        "w< T >.\n"
        "..[M/..\n"
        ".. w ..\n" );

    // force the move aggregate active so the tank won't be woken up:
    game.getMoveAggregate()->onStateChanged( QAbstractAnimation::Running, QAbstractAnimation::Stopped );

    testFuturePushToward(  90, &game );
    testFuturePushToward( 180, &game );
    testFuturePushToward( 270, &game );
    testFuturePushToward(   0, &game );
}

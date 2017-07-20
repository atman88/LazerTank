#include <iostream>
#include "../testmain.h"
#include "controller/game.h"
#include "controller/animationstateaggregator.h"
#include "controller/movecontroller.h"
#include "model/tank.h"
#include "model/shotmodel.h"

using namespace std;

void TestMain::testGameMove()
{
    initGame(
      "T..F......\n"
      "..........\n"
      ".M........\n"
      "..........\n"
      "..........\n" );
    Game& game = mRegistry->getGame();
    Board* board = game.getBoard();
    cout << "board " << board->getWidth() << "x" << board->getHeight() << endl;

    const PieceSet& tiles = board->getPieceManager().getPieces();
    QCOMPARE( (int) tiles.size(), 1 );
    QCOMPARE( (*tiles.begin())->encodedPos(), Piece::encodePos(1,2));

    // check off-board values;
    QCOMPARE( game.canPlaceAt(TANK,ModelPoint(-1, 0),270), false );
    QCOMPARE( game.canPlaceAt(TANK,ModelPoint( 0,-1),  0), false );

    QCOMPARE( game.canPlaceAt( TANK, board->getTankStartVector(), 0 ), true );
}

void testCannonAt( int tankCol, int tankRow, GameRegistry* registry )
{
    cout << "testCannonAt " << tankCol << "," << tankRow << endl;
    registry->getTank().onBoardLoaded( ModelPoint(tankCol,tankRow) );
    registry->getCannonShot().reset();

    registry->getGame().sightCannons();

    SignalReceptor killedReceptor;
    ShotModel& shot = registry->getCannonShot();
    QObject::connect( &shot, &ShotModel::tankKilled, &killedReceptor, &SignalReceptor::receive );
    for( int seq = shot.getSequence().toInt(); seq < 5 && !killedReceptor.mReceived; ++seq ) {
        shot.setSequence( QVariant(seq) );
    }
    QObject::disconnect( &shot, 0, &killedReceptor, 0 );

    QCOMPARE( killedReceptor.mReceived, true );
}

void TestMain::testGameCannon()
{
    initGame(
      "v...<\n"
      ".....\n"
      "..T..\n"
      ".....\n"
      ">...^\n" );

    testCannonAt( 2, 0, mRegistry );
    testCannonAt( 4, 2, mRegistry );
    testCannonAt( 2, 4, mRegistry );
    testCannonAt( 0, 2, mRegistry );
}

void testFuturePushToward( int direction, GameRegistry* registry )
{
    cout << "testFuturePushToward " << direction << endl;

    MoveController& moveController = registry->getMoveController();
    moveController.getMoves().reset();

    // move twice; first may merely rotate the tank but more importantly primes the move list:
    moveController.move( direction );
    moveController.move( direction );
    Game& game = registry->getGame();
    QVERIFY( game.getDeltaPieces()->size() > 0 );
    game.undoLastMove();
    QCOMPARE( (int) game.getDeltaPieces()->size(), 0 );
}

void TestMain::testGamePush()
{
    initGame(
        ".. m ..\n"
        ".. M ..\n"
        ".. . ..\n"
        "w< T >.\n"
        "..[M/..\n"
        ".. w ..\n" );

    // force the move aggregate active so the tank won't be woken up:
    mRegistry->getMoveAggregate().onStateChanged( QAbstractAnimation::Running, QAbstractAnimation::Stopped );

    testFuturePushToward(  90, mRegistry );
    testFuturePushToward( 180, mRegistry );
    testFuturePushToward( 270, mRegistry );
    testFuturePushToward(   0, mRegistry );
}

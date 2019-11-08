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
    Game& game = mRegistry.getGame();
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

class TestTank : public Tank
{
public:
    void gotoVector( const ModelVector& v )
    {
        mVector = v;
        TankView::reset( v );
    }
};

void testCannon( GameRegistry& registry )
{
    cout << "testCannonAt " << registry.getTank().getCol() << "," << registry.getTank().getRow() << endl;
    registry.getCannonShot().reset();

    registry.getGame().sightCannons();

    SignalReceptor killedReceptor;
    ShotModel& shot = registry.getCannonShot();
    QObject::connect( &shot, &ShotModel::tankKilled, &killedReceptor, &SignalReceptor::receive );
    for( int currentTime = 0; currentTime <= 60*5 && !killedReceptor.mReceived; currentTime += 60 ) {
        shot.onTimeChanged( currentTime );
    }
    QObject::disconnect( &shot, nullptr, &killedReceptor, nullptr );

    QCOMPARE( killedReceptor.mReceived, true );
}

void TestMain::testGameCannon()
{
    auto tank = new TestTank();
    mRegistry.injectTank( tank );
    initGame(
      "v...<\n"
      ".....\n"
      "..T..\n"
      ".....\n"
      ">...^\n" );

    tank->gotoVector( ModelVector( 2, 0 ) ); testCannon( mRegistry );
    tank->gotoVector( ModelVector( 4, 2 ) ); testCannon( mRegistry );
    tank->gotoVector( ModelVector( 2, 4 ) ); testCannon( mRegistry );
    tank->gotoVector( ModelVector( 0, 2 ) ); testCannon( mRegistry );
}

void testFuturePushToward( int direction, GameRegistry& registry )
{
    cout << "testFuturePushToward " << direction << endl;

    MoveController& moveController = registry.getMoveController();
    moveController.getMoves().reset();

    // move twice; first may merely rotate the tank but more importantly primes the move list:
    moveController.move( direction );
    moveController.move( direction );
    Game& game = registry.getGame();
    QVERIFY( !game.getDeltaPieces()->empty() );
    moveController.undoLastMove();
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
    mRegistry.getMoveAggregate().onStateChanged( QAbstractAnimation::Running, QAbstractAnimation::Stopped );

    testFuturePushToward(  90, mRegistry );
    testFuturePushToward( 180, mRegistry );
    testFuturePushToward( 270, mRegistry );
    testFuturePushToward(   0, mRegistry );
}

void TestMain::testIsMasterBoard()
{
    class MyTestGame : public Game
    {
    public:
        Board* getDeltaFutureBoard()
        {
            return mFutureDelta.getFutureBoard();
        }

        void enableFutureDelta()
        {
            mFutureDelta.enable();
        }
    };

    auto game = new MyTestGame();
    mRegistry.injectGame( game );
    initGame( "T\n" );
    QVERIFY( game->isMasterBoard( game->getBoard()) );
    QVERIFY( game->isMasterBoard(game->getBoard(false)) );
    QVERIFY( game->isMasterBoard(game->getBoard(true )) );
    QVERIFY( game->isMasterBoard(game->getDeltaFutureBoard()) == false );

    game->enableFutureDelta();

    QVERIFY( game->isMasterBoard(game->getBoard(false)) == true  );
    QVERIFY( game->isMasterBoard(game->getBoard(true )) == false );
    QVERIFY( game->isMasterBoard(game->getDeltaFutureBoard()) == false );
}

#include <iostream>
#include "../testmain.h"
#include "model/futureshotpath.h"
#include "controller/game.h"
#include "controller/movecontroller.h"
#include "model/board.h"
#include "model/tank.h"
#include "model/board.h"

using namespace std;

void TestMain::testFutureShotPath()
{
    initGame(
      "[M/W[\\M\n"
      " w . <\n"
      " M . .\n"
      " T m .\n" );

    FutureShotPathManager manager;
    manager.setParent( &mRegistry );

    MovePiece move( MOVE, mRegistry.getTank().getCol(), mRegistry.getTank().getRow(), 0, 6 );
    const FutureShotPath* path = manager.updateShots( 0, &move);
    QCOMPARE( path->getUID(), move.getShotPathUID() );

    Game& game = mRegistry.getGame();
    Board* futureBoard = game.getBoard(true);

    // verify that this is the future board:
    QCOMPARE(game.isMasterBoard(futureBoard), false);

    QCOMPARE( futureBoard->getPieceManager().typeAt( ModelPoint(2,3) ), CANNON );

    int previousShotCount = move.getShotCount();
    move.setShotCount( 5 );
    manager.updateShots( previousShotCount, &move);
    QCOMPARE( futureBoard->getPieceManager().typeAt( ModelPoint(2,3) ), CANNON );

    previousShotCount = move.getShotCount();
    move.setShotCount( 4 );
    manager.updateShots( previousShotCount, &move );
    QCOMPARE( futureBoard->getPieceManager().typeAt( ModelPoint(2,3) ), NONE );

    move.setShotCount( 0 );
    manager.updateShots( previousShotCount, &move );
    QCOMPARE( (int) game.getDeltaPieces()->size(), 0 );
}

void TestMain::testFutureShotThruStationaryTank()
{
    initGame(
      "[S/ M[T<\n"
      "[S\\.[/S" );
    mRegistry.getMoveController().move(180);
    mRegistry.getMoveController().fire();
    QVERIFY( !mRegistry.getGame().getDeltaPieces() );
}

/**
 * @brief Verify that the master tank point doesn't cause a false positive hit detection
 */
void TestMain::testFutureShotThruMasterTank()
{
    initGame(
      "W\n"
      "T\n"
      ".\n" );
    MoveController& moveController = mRegistry.getMoveController();
    moveController.move(180); // rotate down
    moveController.move(180); // move down -> 0,2
    moveController.move(  0); // rotate up
    moveController.fire();
    QCOMPARE( mRegistry.getGame().getBoard(true)->tileAt( ModelPoint(0,0) ), WOOD_DAMAGED );
}

void TestMain::testFutureMultiShotThruTank()
{
    initGame(
      ". . .\n"
      ". M .\n"
      ".[T<.\n"
      ". . .\n" );

    MoveController& moveController = mRegistry.getMoveController();
    moveController.move(270);
    moveController.move(0);
    moveController.move(0);
    moveController.move(0);
    moveController.move(90);
    moveController.move(90);
    moveController.move(180);
    moveController.fire(2);
    QCOMPARE( TILE, mRegistry.getGame().getBoard(true)->getPieceManager().pieceAt(ModelPoint(1,3))->getType() );
    SimplePiece key(MOVE, 1, 3 );
    auto it = mRegistry.getGame().getDeltaPieces()->find( &key );
    QVERIFY( it != mRegistry.getGame().getDeltaPieces()->end() );
    cout << "type=" << (*it)->getType() << endl;
    QVERIFY( (*it)->getType() == TILE_FUTURE_INSERT );
}

class MyTestGame : public Game
{
public:
    void enableFuture()
    {
        mFutureDelta.enable();
    }
};

void TestMain::testFutureShotPushId()
{
    auto game = new MyTestGame();
    mRegistry.injectGame( game );
    initGame(
      "[T>.M....\n" );
    MoveController& moveController = mRegistry.getMoveController();
    game->enableFuture();
    const PieceSet& boardPieces = mRegistry.getGame().getBoard(true)->getPieceManager().getPieces();
    QCOMPARE( (*boardPieces.begin())->getPushedId(), 0 );

    moveController.fire(2);
    QCOMPARE( (*boardPieces.begin())->getPushedId(), 2 );

    moveController.move(90);
    moveController.fire(2);
    QCOMPARE( (*boardPieces.begin())->getPushedId(), 4 );

    moveController.undoLastMove();
    QCOMPARE( (*boardPieces.begin())->getPushedId(), 2 );

    moveController.move(90);
    moveController.fire(2);
    QCOMPARE( (*boardPieces.begin())->getPushedId(), 4 );
}

class SleepingMoveController : public MoveController
{
public:
    bool canWakeup() override {
        return false;
    }
};

void TestMain::testFutureShotPartialUndo()
{
    auto game = new MyTestGame();
    mRegistry.injectGame( game );
    auto moveController = new SleepingMoveController();
    mRegistry.injectMoveController( moveController );
    initGame(
      "..M.[T<\n" );
    game->enableFuture();

    moveController->fire(2);
    moveController->fire(1);

    const PieceSet& boardPieces = game->getBoard(true)->getPieceManager().getPieces();
    QVERIFY( ModelPoint(1,0).equals( *(*boardPieces.begin()) ) );
}

void TestMain::testFutureShotPushIdWater()
{
    auto game = new MyTestGame();
    mRegistry.injectGame( game );
    auto moveController = new SleepingMoveController();
    mRegistry.injectMoveController( moveController );
    initGame(
      "[T>M..w\n" );
    game->enableFuture();
    const PieceSet& boardPieces = game->getBoard(true)->getPieceManager().getPieces();
    QCOMPARE( (*boardPieces.begin())->getPushedId(), 0 );

    moveController->fire(3);
    moveController->fire(2);
    QCOMPARE( (*boardPieces.begin())->getPushedId(), 2 );
}

void TestMain::testFutureShot2PushIdWater()
{
    auto game = new MyTestGame();
    mRegistry.injectGame( game );
    auto moveController = new SleepingMoveController();
    mRegistry.injectMoveController( moveController );
    initGame(
      "[T>.M..wM.\n" );
    game->enableFuture();
    PieceSetManager& pieceManager = game->getBoard(true)->getPieceManager();

    moveController->move(90);
    moveController->fire(4);
    QCOMPARE( pieceManager.pieceAt( ModelPoint(7,0) )->getPushedId(), 4 );
    moveController->fire(3);
    QCOMPARE( pieceManager.pieceAt( ModelPoint(6,0) )->getPushedId(), 0 );
    moveController->fire(0);
    QVERIFY( mRegistry.getGame().getDeltaPieces()->empty() );
}

void TestMain::testFutureShotTankKill()
{
    auto game = new MyTestGame();
    mRegistry.injectGame( game );
    auto moveController = new SleepingMoveController();
    mRegistry.injectMoveController( moveController );
    initGame(
      "[S/ .  [\\S\n"
      " .  <    .\n"
      "[T^[S\\[/S\n" );
    game->enableFuture();
    const PieceSet& boardPieces = game->getBoard(true)->getPieceManager().getPieces();

    moveController->fire(2);
    QVERIFY( boardPieces.empty() );

    moveController->fire(1);
    QCOMPARE( (*boardPieces.begin())->getPushedId(), 1 );
}

#include <iostream>
#include <QTest>
#include "controller/Game.h"

class TestGame : public QObject
{
    Q_OBJECT

private slots:
    void testMove();
    void testMoveIntent();
};

void TestGame::testMove()
{
    Board board(":/maps/testsimple.txt");
    cout << "board " << board.getWidth() << "x" << board.getHeight() << std::endl;
    Game game( &board );
    QCOMPARE( game.getTankX(), 0 );
    QCOMPARE( game.getTankY(), 0 );
    QCOMPARE( game.canPlaceAt(-1,0), false );
    QCOMPARE( game.canPlaceAt(0,-1), false );
    cout << "tank " << game.getTankX() << "," << game.getTankY() << std::endl;
    QCOMPARE( game.canPlaceAt(game.getTankX(),game.getTankY()), true );

    QCOMPARE( game.addMove( 90 ), true );
    QVariant v = game.property( "IntentList" );
    IntentList intents = v.value<IntentList>();
    QCOMPARE( intents.size(), 1UL );

    QCOMPARE( game.addMove( 180 ), true );
    QCOMPARE( game.addMove( 270 ), true );
    QCOMPARE( game.addMove( 0 ), true );
    QCOMPARE( game.addMove( 90 ), true );
    QCOMPARE( game.addMove( 90 ), true );
    v = game.property( "IntentList" );
    intents = v.value<IntentList>();
    QCOMPARE( intents.size(), 6UL );

    intents.sort();
    IntentList::iterator it = intents.begin();
    QCOMPARE(it->getAngle(),       0 ); QCOMPARE( it->encodedPos(), Intent::encodePos(0,0));
    QCOMPARE((++it)->getAngle(),  90 ); QCOMPARE( it->encodedPos(), Intent::encodePos(1,0));
    QCOMPARE((++it)->getAngle(),  90 ); QCOMPARE( it->encodedPos(), Intent::encodePos(1,0));
    QCOMPARE((++it)->getAngle(),  90 ); QCOMPARE( it->encodedPos(), Intent::encodePos(2,0));
    QCOMPARE((++it)->getAngle(), 270 ); QCOMPARE( it->encodedPos(), Intent::encodePos(0,1));
    QCOMPARE((++it)->getAngle(), 180 ); QCOMPARE( it->encodedPos(), Intent::encodePos(1,1));

    // no-op:
    game.onTankMoved(0,0);
    v = game.property( "IntentList" );
    intents = v.value<IntentList>();
    QCOMPARE( intents.size(), 6UL );

    game.onTankMoved(1,0);
    v = game.property( "IntentList" );
    intents = v.value<IntentList>();
    QCOMPARE( intents.size(), 5UL );

    game.onTankMoved(1,1);
    v = game.property( "IntentList" );
    intents = v.value<IntentList>();
    QCOMPARE( intents.size(), 4UL );
}

QTEST_MAIN(TestGame)
#include "testgame.moc"

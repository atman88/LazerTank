#include <iostream>
#include <QTest>
#include "controller/Game.h"

class TestGame : public QObject
{
    Q_OBJECT

private slots:
    void testMove();
};

void TestGame::testMove()
{
    Board board(":/maps/testsimple.txt");
    cout << "board " << board.getWidth() << "x" << board.getHeight() << std::endl;

    QVariant v = board.property("tiles");
    PieceSet tiles = v.value<PieceSet>();
    QCOMPARE( tiles.size(), 1UL );
    QCOMPARE( tiles.begin()->encodedPos(), Piece::encodePos(1,2));

    Game game( &board );
    QCOMPARE( game.getTankX(), 0 );
    QCOMPARE( game.getTankY(), 0 );
    QCOMPARE( game.canPlaceAt(-1,0), false );
    QCOMPARE( game.canPlaceAt(0,-1), false );
    cout << "tank " << game.getTankX() << "," << game.getTankY() << std::endl;
    QCOMPARE( game.canPlaceAt(game.getTankX(),game.getTankY()), true );

    QCOMPARE( game.addMove( 90 ), true );
    v = game.property( "MoveList" );
    PieceList moves = v.value<PieceList>();
    QCOMPARE( moves.size(), 1UL );

    QCOMPARE( game.addMove( 180 ), true );
    QCOMPARE( game.addMove( 270 ), true );
    QCOMPARE( game.addMove( 0 ), true );
    QCOMPARE( game.addMove( 90 ), true );
    QCOMPARE( game.addMove( 90 ), true );
    v = game.property( "MoveList" );
    moves = v.value<PieceList>();
    QCOMPARE( moves.size(), 6UL );

    moves.sort();
    PieceList::iterator it = moves.begin();
    QCOMPARE(it->getAngle(),       0 ); QCOMPARE( it->encodedPos(), Piece::encodePos(0,0));
    QCOMPARE((++it)->getAngle(),  90 ); QCOMPARE( it->encodedPos(), Piece::encodePos(1,0));
    QCOMPARE((++it)->getAngle(),  90 ); QCOMPARE( it->encodedPos(), Piece::encodePos(1,0));
    QCOMPARE((++it)->getAngle(),  90 ); QCOMPARE( it->encodedPos(), Piece::encodePos(2,0));
    QCOMPARE((++it)->getAngle(), 270 ); QCOMPARE( it->encodedPos(), Piece::encodePos(0,1));
    QCOMPARE((++it)->getAngle(), 180 ); QCOMPARE( it->encodedPos(), Piece::encodePos(1,1));

    // no-op:
    game.onTankMoved(0,0);
    v = game.property( "MoveList" );
    moves = v.value<PieceList>();
    QCOMPARE( moves.size(), 6UL );

    game.onTankMoved(1,0);
    v = game.property( "MoveList" );
    moves = v.value<PieceList>();
    QCOMPARE( moves.size(), 5UL );

    game.onTankMoved(1,1);
    v = game.property( "MoveList" );
    moves = v.value<PieceList>();
    QCOMPARE( moves.size(), 4UL );
}

QTEST_MAIN(TestGame)
#include "testgame.moc"

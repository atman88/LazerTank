#include "../testmain.h"
#include "model/piecelistmanager.h"

void TestMain::testPieceListManager()
{
    PieceListManager manager;

    manager.append( MOVE, ModelPoint(1, 2),  90 );
    manager.append( MOVE, ModelPoint(1, 2), 270 );
    manager.append( MOVE, ModelPoint(3, 4),   0 );
    manager.append( MOVE, ModelPoint(3, 4), 180 );
    QCOMPARE( manager.toSet()->size(), 2UL );
    QCOMPARE( manager.toMultiSet()->size(), 4UL );
    manager.eraseBack();
    QCOMPARE( manager.toSet()->size(), 1UL );
    QCOMPARE( manager.toMultiSet()->size(), 3UL );
    manager.eraseBack();
    QCOMPARE( manager.toSet()->size(), 1UL );
    QCOMPARE( manager.toMultiSet()->size(), 2UL );

    manager.eraseFront();
    QCOMPARE( manager.toSet()->size(), 0UL );
    QCOMPARE( manager.toMultiSet()->size(), 1UL );
    manager.eraseBack();
    QCOMPARE( manager.toSet()->size(), 0UL );
    QCOMPARE( manager.toMultiSet()->size(), 0UL );
}

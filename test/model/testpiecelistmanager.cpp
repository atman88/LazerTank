#include "../testmain.h"
#include "model/piecelistmanager.h"

void TestMain::testPieceListManager()
{
    PieceListManager manager;

    manager.append( MOVE, ModelPoint(1, 2),  90 );
    manager.append( MOVE, ModelPoint(1, 2), 270 );
    manager.append( MOVE, ModelPoint(3, 4),   0 );
    manager.append( MOVE, ModelPoint(3, 4), 180 );
    QCOMPARE( (int) manager.toSet()->size(), 2 );
    QCOMPARE( (int) manager.toMultiSet()->size(), 4 );
    manager.eraseBack();
    QCOMPARE( (int) manager.toSet()->size(), 1 );
    QCOMPARE( (int) manager.toMultiSet()->size(), 3 );
    manager.eraseBack();
    QCOMPARE( (int) manager.toSet()->size(), 1 );
    QCOMPARE( (int) manager.toMultiSet()->size(), 2 );

    manager.eraseFront();
    QCOMPARE( (int) manager.toSet()->size(), 0 );
    QCOMPARE( (int) manager.toMultiSet()->size(), 1 );
    manager.eraseBack();
    QCOMPARE( (int) manager.toSet()->size(), 0 );
    QCOMPARE( (int) manager.toMultiSet()->size(), 0 );
}

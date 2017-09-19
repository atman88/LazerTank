#include "../testmain.h"
#include "model/level.h"

void TestMain::testNextLevel()
{
    LevelList levelList;
    for( int number = 1; number <= 12; ++number ) {
        levelList.addLevel( number, 10, 10 );
    }

    QVERIFY( levelList.nextLevel(-1) ==  1 );
    QVERIFY( levelList.nextLevel( 0) ==  1 );
    QVERIFY( levelList.nextLevel( 1) ==  2 );
    QVERIFY( levelList.nextLevel( 2) ==  3 );
    QVERIFY( levelList.nextLevel(11) == 12 );
    QVERIFY( levelList.nextLevel(12) ==  0 );
}

void verifyFind( LevelList& list, int number )
{
    const Level* level = list.find(number);
    QVERIFY( level != 0 );
    QVERIFY( level->getNumber() == number );
}

void TestMain::testLevelFind()
{
    LevelList list;
    for( int number = 1; number <= 12; ++number ) {
        list.addLevel( number, 10, 10 );
    }

    verifyFind( list,  1 );
    verifyFind( list, 11 );
    verifyFind( list, 12 );
}

void TestMain::testLevelCompleted()
{
    LevelList list;
    list.addLevel( 1, 10, 10 );
    QVERIFY( list.at(0)->getCompletedCount() == 0 );
    QSignalSpy levelSpy( &list, SIGNAL(levelUpdated(const QModelIndex&)) );
    list.setCompleted( 1, 1 );
    QVERIFY( list.at(0)->getCompletedCount() > 0 );
    QVERIFY( levelSpy.size() == 1 );
}

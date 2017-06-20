#include <iostream>
#include "../testmain.h"
#include "view/levelchooser.h"

using namespace std;

void TestMain::testNextLevel()
{
    LevelChooser chooser;
    for( int number = 1; number <= 12; ++number ) {
        const_cast<LevelList&>(chooser.getList()).addLevel( number, 10, 10 );
    }

    QVERIFY( chooser.nextLevel(-1) ==  1 );
    QVERIFY( chooser.nextLevel( 0) ==  1 );
    QVERIFY( chooser.nextLevel( 1) ==  2 );
    QVERIFY( chooser.nextLevel( 2) ==  3 );
    QVERIFY( chooser.nextLevel(11) == 12 );
    QVERIFY( chooser.nextLevel(12) ==  0 );
}

void verifyFind( LevelChooser& chooser, int number )
{
    const Level* level = chooser.find(number);
    QVERIFY( level != 0 );
    QVERIFY( level->getNumber() == number );
}

void TestMain::testLevelChooserFind()
{
    LevelChooser chooser;
    for( int number = 1; number <= 12; ++number ) {
        const_cast<LevelList&>(chooser.getList()).addLevel( number, 10, 10 );
    }

    verifyFind( chooser,  1 );
    verifyFind( chooser, 11 );
    verifyFind( chooser, 12 );
}

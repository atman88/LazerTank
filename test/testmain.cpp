#include <QSignalSpy>
#include <QString>

#include "testmain.h"
#include "controller/game.h"

QTEST_MAIN(TestMain)

void TestMain::initGame( Game& game, const char* map )
{
    QSignalSpy loadSpy( &game, &Game::boardLoaded );
    game.init(0);
    QTextStream s( map );
    game.getBoard()->load( s );
    QCOMPARE( loadSpy.wait(1000), true );
}

void TestMain::initGame(Game& game, QTextStream& map )
{
    QSignalSpy loadSpy( &game, &Game::boardLoaded );
    game.init(0);
    game.getBoard()->load( map );
    QCOMPARE( loadSpy.wait(1000), true );
}

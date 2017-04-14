#include <QSignalSpy>
#include <QString>

#include "testmain.h"
#include "controller/gameregistry.h"
#include "controller/game.h"
#include "util/workerthread.h"


QTEST_MAIN(TestMain)

TestMain::TestMain() : mStream(0), mRegistry(0)
{
}

TestMain::~TestMain()
{
    if ( mRegistry ) {
        delete mRegistry;
    }
    if ( mStream ) {
        delete mStream;
    }
}

void TestMain::initGame( const char* map )
{
    mStream = new QTextStream( map );
    initGame( *mStream );
}

void TestMain::initGame( QTextStream& map )
{
    mRegistry = new GameRegistry();
    Game& game = mRegistry->getGame();
    QSignalSpy loadSpy( &game, &Game::boardLoaded );
    game.init(mRegistry);
    game.getBoard()->load( map );
    QCOMPARE( loadSpy.wait(1000), true );
}

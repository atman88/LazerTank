#include <iostream>
#include <set>
#include <QSignalSpy>
#include <QString>

#include "testmain.h"
#include "controller/gameregistry.h"
#include "controller/game.h"
#include "util/workerthread.h"

TestRegistry::~TestRegistry()
{
    cleanup();
}

void TestRegistry::cleanup()
{
    mWorker.purge();

#define DECL_CLEAN(name) { if ( m##name != nullptr ) { delete m##name; m##name=nullptr; } }
    DECL_CLEAN(Game)
    DECL_CLEAN(SpeedController)
    DECL_CLEAN(MoveController)
    DECL_CLEAN(PathFinderController)
    DECL_CLEAN(MoveAggregate)
    DECL_CLEAN(ShotAggregate)
    DECL_CLEAN(BoardPool)
    DECL_CLEAN(Tank)
    DECL_CLEAN(ActiveCannon)
    DECL_CLEAN(TankPush)
    DECL_CLEAN(ShotPush)
    DECL_CLEAN(LevelList)
    DECL_CLEAN(Recorder)
    DECL_CLEAN(Persist)
}

QTEST_GUILESS_MAIN(TestMain)

TestMain::TestMain() : mStream(0)
{
}

TestMain::~TestMain()
{
    cleanup();
}

void TestMain::initGame( const char* map )
{
    mStream = new QTextStream( map );
    initGame( *mStream );
}

void TestMain::initGame( QTextStream& map )
{
    Game& game = mRegistry.getGame();
    QSignalSpy loadSpy( &game, &Game::boardLoaded );
    game.init(&mRegistry);
    game.getBoard()->load( map );
    QCOMPARE( loadSpy.count(), 1 );
}

TestRegistry* TestMain::getRegistry()
{
    return &mRegistry;
}

void TestMain::cleanup()
{
    mRegistry.cleanup();
    if ( mStream ) {
        delete mStream;
        mStream = 0;
    }
}

DirtySpy::DirtySpy(QObject* object) : QObject(0)
{
    mSignalSpy = new QSignalSpy(this,&DirtySpy::dirty);
    QObject::connect( object, SIGNAL(rectDirty(QRect&)), this, SLOT(rectDirty(QRect&)) );
}

DirtySpy::~DirtySpy()
{
    delete mSignalSpy;
}

bool DirtySpy::wait(int msecs)
{
    return mSignalSpy->wait(msecs);
}

void DirtySpy::rectDirty( QRect& rect __attribute__ ((unused)) )
{
    std::cout << "DirtySpy: dirty" << std::endl;
    emit dirty();
}

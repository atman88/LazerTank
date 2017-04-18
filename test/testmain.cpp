#include <iostream>
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

GameRegistry* TestMain::getRegistry() const
{
    return mRegistry;
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

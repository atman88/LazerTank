#include <iostream>
#include <QFile>
#include <QTimer>

#include "../testmain.h"
#include "../test/util/testasync.h"
#include "util/persist.h"
#include "util/recorder.h"
#include "controller/gameregistry.h"

class TestPersistUpdate : public TestAsync
{
public:
    TestPersistUpdate( Persist& persist ) : mPersist(persist)
    {
    }

    bool condition() override
    {
        return !mPersist.updateInProgress();
    }

private:
    Persist& mPersist;
};

void TestMain::testPersistSizes()
{
    // version 1.0 sizes
    QCOMPARE( sizeof(PersistedLevelIndexFooter), 6UL );
    QCOMPARE( sizeof(PersistedLevelIndex      ), 8UL );
    QCOMPARE( sizeof(PersistedLevelRecord     ), 8UL );
}

Persist* TestMain::setupTestPersist()
{
    Persist* persist = new Persist( "qlttest.sav" );
    mRegistry.injectPersist(persist);

    // start each test with the file non-existant:
    QFile file( persist->getPath() );
    if ( file.exists() && !file.remove() ) {
        std::cout << "remove \"" << qPrintable(file.fileName()) << "\" failed. " << qPrintable(file.errorString()) << std::endl;
        return 0;
    }

    TestPersistUpdate testPersistAsync( *persist );
    persist->init( &mRegistry );
    if ( !testPersistAsync.test() ) {
        return 0;
    }

    return persist;
}

void TestMain::testPersistNew()
{
    if ( Persist* persist = setupTestPersist() ) {
        Recorder& recorder = mRegistry.getRecorder();
        mStream = new QTextStream( "T" );
        mRegistry.getGame().getBoard()->load(*mStream,2);
        recorder.onBoardLoaded( 2 );
        unsigned char data[] = { 80 };
        recorder.setData( sizeof data, data );
        QSignalSpy persistSpy( persist, SIGNAL(levelSetComplete(int)) );
        persist->onLevelUpdated( 2 );
        QVERIFY(persistSpy.wait(2000));
        TestPersistUpdate testPersistAsync( *persist );
        QVERIFY( testPersistAsync.test() ); // wait for runnable to be disposed

        persist->init( &mRegistry );
        QVERIFY( testPersistAsync.test() );
        QCOMPARE( persistSpy.takeFirst().at(0).toInt(), 2 );
    } else {
        QFAIL( "init failure" );
    }
}

void persistLevel( Recorder& recorder, int level, int count, Persist& persist )
{
    recorder.onBoardLoaded( level );
    for( int i = count; --i >= 0; ) {
        recorder.recordMove(true,90);
    }
    std::cout << "recorder count=" << recorder.getCount() << std::endl;

    persist.onLevelUpdated( level );
    TestPersistUpdate testUpdate( persist );
    QVERIFY2( testUpdate.test(), "file not updated" );
}

void TestMain::testPersistReplace()
{
    if ( Persist* persist = setupTestPersist() ) {
        Recorder& recorder = mRegistry.getRecorder();
        persistLevel( recorder, 3, 3, *persist );
        persistLevel( recorder, 2, 2, *persist );
        persistLevel( recorder, 3, 1, *persist );

        // read them back
        TestPersistUpdate testPersistAsync( *persist );
        persist->init( &mRegistry );
        QVERIFY( testPersistAsync.test() );

        PersistLevelLoader* loader = persist->getLevelLoader(2);
        char buf[2];
        QSignalSpy loaderSpy( loader, SIGNAL(dataReady()) );
        loader->load( buf, sizeof buf );
        QVERIFY( loaderSpy.wait(1000) );
    } else {
        QFAIL( "init failure" );
    }
}
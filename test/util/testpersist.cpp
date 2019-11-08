#include <iostream>
#include <QFile>
#include <QTimer>

#include "../testmain.h"
#include "../test/util/testasync.h"
#include "util/persist.h"
#include "util/recorder.h"
#include "util/recorderprivate.h"
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
    QVERIFY( sizeof(PersistedLevelIndexFooter) == 6 );
    QVERIFY( sizeof(PersistedLevelIndex      ) == 8 );
    QVERIFY( sizeof(PersistedLevelRecord     ) == 8 );
}

Persist* TestMain::setupTestPersist()
{
    auto persist = new Persist( "qlttest.sav" );
    mRegistry.injectPersist(persist);

    // start each test with the file non-existant:
    QFile file( persist->getPath() );
    if ( file.exists() && !file.remove() ) {
        std::cout << "remove \"" << qPrintable(file.fileName()) << "\" failed. " << qPrintable(file.errorString()) << std::endl;
        return nullptr;
    }

    TestPersistUpdate testPersistAsync( *persist );
    persist->init( &mRegistry );
    if ( !testPersistAsync.test() ) {
        return nullptr;
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
        recorder.recordMove(true,90);
        QSignalSpy persistSpy( persist, SIGNAL(levelSetComplete(int,int)) );
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
    std::cout << "recorder count=" << recorder.getAvailableCount() << std::endl;

    persist.onLevelUpdated( level );
    TestPersistUpdate testUpdate( persist );
    QVERIFY2( testUpdate.test(10*60*1000), "file not updated" );
}

void TestMain::testPersistReplace()
{
    if ( Persist* persist = setupTestPersist() ) {
        auto recorder_p = new RecorderPrivate( 2 );
        mRegistry.injectRecorder( new Recorder(recorder_p) );
        Recorder& recorder = mRegistry.getRecorder();
        persistLevel( recorder, 3, 3, *persist );
        persistLevel( recorder, 2, 2, *persist );
        persistLevel( recorder, 3, 1, *persist );

        // read back level 2
        recorder.onBoardLoaded( 2 );
        RecorderSource* source = recorder.source();
        QCOMPARE( source->getCount(), 0 );
        PersistLevelLoader* loader = persist->getLevelLoader(2);
        Q_ASSERT(loader);
        QSignalSpy loaderSpy( loader, SIGNAL(dataReady()) );
        loader->load( *recorder_p );
        QVERIFY( loaderSpy.wait(1000) );
        delete loader;
        QCOMPARE( source->getCount(), 2 );
        delete source;
    } else {
        QFAIL( "init failure" );
    }
}

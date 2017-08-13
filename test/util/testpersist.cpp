#include <iostream>
#include <QFile>
#include <QTimer>

#include "../testmain.h"
#include "util/persist.h"
#include "util/recorder.h"
#include "controller/gameregistry.h"
#include "model/tank.h"

Persist& TestMain::setupTestPersist()
{
    Persist* persist = new Persist( "qlttest.sav" );
    mRegistry.injectPersist(persist);

    // start each test with the file non-existant:
    QFile file( persist->getPath() );
    file.remove();

    return mRegistry.getPersist();
}

void TestMain::testPersistNew()
{
    Persist& persist = setupTestPersist();

    persist.init( &mRegistry ); // this is expected to be a noop given qlttest.sav doesn't exist

    Recorder& recorder = mRegistry.getTank().getRecorder();
    mStream = new QTextStream( "T" );
    Board* board = mRegistry.getGame().getBoard();
    board->load(*mStream,2);
    recorder.onBoardLoaded( *board );
    unsigned char data[] = { 80 };
    recorder.setData( sizeof data, data );
    persist.onLevelUpdated( 2 );

    QSignalSpy persistSpy( &persist, SIGNAL(levelSetComplete(int)) );
    persist.init( &mRegistry );
    QVERIFY(persistSpy.wait(2000));
    std::cout << "fileSize=" << persist.getFileSize() << std::endl;
    QFile file( persist.getPath() );
    QCOMPARE( persist.getFileSize(), file.size() );
    QCOMPARE( persistSpy.takeFirst().at(0).toInt(), 2 );
}

class TestPersistTimer : public QTimer
{
public:
    TestPersistTimer( Persist& persist, QEventLoop& loop ) : QTimer(0), mPersist(persist), mLoop(loop)
    {
        mCreateTime.start();
    }

protected:
    void timerEvent( QTimerEvent* e ) override
    {
        if ( !mPersist.lastUpdateTime().isNull() ) {
            mLoop.exit(1);
        } else if ( mCreateTime.elapsed() >= 1000 ) {
            mLoop.exit(0);
        } else {
            QTimer::timerEvent( e );
        }
    }

    Persist& mPersist;
    QEventLoop& mLoop;
public:
    QTime mCreateTime;
};

void persistLevel( Recorder& recorder, int level, int count, Persist& persist )
{
    Board board;
    QTextStream stream( "[T>..." );
    board.load( stream, level );
    recorder.onBoardLoaded( board );
    for( int i = count; --i >= 0; ) {
        recorder.recordMove(true,90);
    }
    std::cout << "recorder count=" << recorder.getCount() << std::endl;

    persist.onLevelUpdated( level );
    QEventLoop loop;
    TestPersistTimer timer( persist, loop );
    timer.start(0);
    std::cout << "singleshot: " << timer.isSingleShot() << std::endl;
    int rc = loop.exec();
    std::cout << "loop done t=" << timer.mCreateTime.elapsed() << " ut=" << persist.lastUpdateTime().elapsed() << std::endl;
    QVERIFY2(rc != 0,"file not updated");
}

void TestMain::testPersistReplace()
{
    Persist& persist = setupTestPersist();

    Recorder& recorder = mRegistry.getTank().getRecorder();
    persistLevel( recorder, 3, 3, persist );
    persistLevel( recorder, 2, 2, persist );
    persistLevel( recorder, 3, 1, persist );

    QSignalSpy persistSpy( &persist, SIGNAL(levelSetComplete(int)) );
    persist.init( &mRegistry );
    QVERIFY(persistSpy.wait(1000));
    QCOMPARE( persistSpy.takeFirst().at(0).toInt(), 3 );
}

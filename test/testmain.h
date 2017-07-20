#ifndef TESTMAIN_H
#define TESTMAIN_H

#include <QObject>
#include <QTest>
#include <QSignalSpy>

class GameRegistry;

class TestMain : public QObject
{
    Q_OBJECT
public:
    TestMain();
    ~TestMain();

    void initGame( const char* map );
    void initGame( QTextStream& map );

    GameRegistry* getRegistry() const;

private slots:
    void testLevelFind();
    void testNextLevel();

    void testBoardPool();

    void testGameMove();
    void testGameCannon();
    void testGamePush();

    void testPieceListManager();

    void testFutureShotPath();
    void testFutureShotThruMasterTank();

    void testRecorderBitFields();
    void testRecorderRecordSize();
    void testRecorderOverflow();

    void testMultiShotQueued();
    void testMultiShotShotDirty();
    void testMultiShotShooterRelease();
    void testMultiShotShotFinished();
    void testReplay();
    void testMoveFocus();

    void testWorker();

    void testDragTank();
    void testDragPoint();
    void testDragWithMove();

private:
    QTextStream* mStream;
    GameRegistry* mRegistry;
};

/**
 * @brief Wrapper for the dirty signal to allow its passed-by-reference signal arg
 */
class DirtySpy : public QObject
{
    Q_OBJECT

public:
    DirtySpy(QObject* object );
    ~DirtySpy();
    bool wait( int msecs = 5000 );

public slots:
    void rectDirty(QRect& rect);
signals:
    void dirty();

private:
    QSignalSpy* mSignalSpy;
};

class SignalReceptor : public QObject
{
    Q_OBJECT

public:
    SignalReceptor() : QObject(0), mReceived(false)
    {
    }

#if (QT_VERSION >= QT_VERSION_CHECK(5, 7, 0))
    SignalReceptor( const QObject* sender, const char* signal ) : QObject(0), mReceived(false)
    {
        QObject::connect( sender, signal, this, SLOT(receive()) );
    }
#endif

    ~SignalReceptor()
    {
        QObject::disconnect( this );
    }

    bool mReceived;

public slots:
    void receive() { mReceived = true; }
};

#endif // TESTMAIN_H

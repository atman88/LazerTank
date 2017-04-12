#ifndef TESTMAIN_H
#define TESTMAIN_H

#include <QObject>
#include <QTest>

class Game;

class TestMain : public QObject
{
    Q_OBJECT
public:
    TestMain() {}
    ~TestMain() {}

    void initGame( Game& game, const char* map );
    void initGame( Game& game, QTextStream& map );

private slots:
    void testGameMove();
    void testGameCannon();
    void testGamePush();

    void testPieceListManager();

    void testFutureShotPath();
    void testFutureShotThruMasterTank();

    void testRecorderBitFields();
    void testRecorderRecordSize();
    void testRecorderOverflow();

    void testReplay();
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

#ifndef TESTMAIN_H
#define TESTMAIN_H

#include <QObject>
#include <QTest>

class TestMain : public QObject
{
    Q_OBJECT
public:
    TestMain() {}
    ~TestMain() {}

private slots:
    void testMove();
    void testCannon();
    void testPieceListManager();
    void testPush();
    void testFutureShotPath();
    void testRecorder();
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

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
};

class SignalReceptor : public QObject
{
    Q_OBJECT

public:
    SignalReceptor() : QObject(0), mReceived(false) {}
    bool mReceived;

public slots:
    void receive() { mReceived = true; }

};


#endif // TESTMAIN_H

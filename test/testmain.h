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
};

#endif // TESTMAIN_H

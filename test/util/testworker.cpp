#include <iostream>
#include "../testmain.h"
#include "controller/gameregistry.h"

using namespace std;

class TestRunnable : public BasicRunnable
{
public:
    TestRunnable() : mStarted(0)
    {
    }

    ~TestRunnable()
    {
    }

    void run() override
    {
        cout << "worker starting" << endl;
        mStarted = true;
        QThread::currentThread()->msleep(1000);
        cout << "worker exiting" << endl;
    }

    bool mStarted;
};

/**
 * @brief test destroying the worker while running
 */
void TestMain::testWorker()
{
    TestRunnable runnable;
    GameRegistry registry;
    registry.getWorker().doWork( &runnable );
    QThread::currentThread()->msleep(10);
    QVERIFY2( runnable.mStarted, "test runnable didn't run" );
}

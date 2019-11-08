#include <iostream>
#include "../testmain.h"
#include "controller/gameregistry.h"
#include "../util/testasync.h"

using namespace std;

class TestRunnable : public BasicRunnable
{
public:
    TestRunnable() : mStarted(false)
    {
    }

    ~TestRunnable() override = default;

    void run() override
    {
        cout << "worker starting" << endl;
        mStarted = true;
        QThread::msleep(10);
        cout << "worker exiting" << endl;
    }

    bool mStarted;
};

class TestWorker : public TestAsync
{
public:
    TestWorker( TestRunnable& runnable ) : mRunnable(runnable)
    {
    }

    bool condition() override {
        return mRunnable.mStarted;
    }

    TestRunnable& mRunnable;
};

/**
 * @brief test destroying the worker while running
 */
void TestMain::testWorker()
{
    std::shared_ptr<Runnable> sharedRunnable;
    auto runnable = new TestRunnable();
    sharedRunnable.reset( runnable );
    TestWorker testWorker( *runnable );
    mRegistry.getWorker().doWork( sharedRunnable );
    QVERIFY( testWorker.test() );
    mRegistry.getWorker().shutdown();
}

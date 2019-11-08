#include <iostream>
#include "workerthread.h"

class SharedRunnableWrapper : public BasicRunnable
{
public:
    SharedRunnableWrapper( std::shared_ptr<Runnable> sharedRunnable ) : mSharedRunnable(sharedRunnable)
    {
    }

    void run() override
    {
        mSharedRunnable.get()->runInternal();
    }

    bool deleteWhenDone() override
    {
        return true;
    }

private:
    std::shared_ptr<Runnable> mSharedRunnable;
};

WorkerThread::WorkerThread() : mThread(nullptr), mShuttingDown(false)
{
}

void WorkerThread::doWork( Runnable* runnable )
{
    if ( !mShuttingDown ) {
        mPendingMutex.lock();

        if ( mPending.empty() && mThread && mThread->joinable() ) {
            std::thread* t = mThread;
            mThread = nullptr;

            mPendingMutex.unlock();
            t->join();
            delete t;
            mPendingMutex.lock();
        }

        mPending.push_back( runnable );

        if ( !mThread ) {
            mThread = new std::thread( [this] { run(); } );
        }
        mPendingMutex.unlock();
    }
}

void WorkerThread::doWork(const std::shared_ptr<Runnable>& sharedRunnable)
{
    doWork( new SharedRunnableWrapper(sharedRunnable) );
}

void WorkerThread::shutdown()
{
    if ( !mShuttingDown ) {
        mShuttingDown = true;
        {   std::lock_guard<std::mutex> guard(mPendingMutex); }

        if ( mThread ) {
            if ( mThread->joinable() ) {
                mThread->join();
            }
            delete mThread;
            mThread = nullptr;
        }
    }
}

void WorkerThread::purge()
{
    shutdown();
    mShuttingDown = false;
}

Runnable* WorkerThread::getCurrentRunnable()
{
    std::lock_guard<std::mutex> guard(mPendingMutex);

    if ( !mPending.empty() ) {
        return mPending.front();
    }
    return nullptr;
}

void WorkerThread::dequeueCurrentRunnable()
{
    std::lock_guard<std::mutex> guard(mPendingMutex);

    mPending.pop_front();

//    if ( mPending.size() ) {
//        std::cout << "WorkerThread: " << mPending.size() << " additional runnables pending" << std::endl;
//    }
}

void WorkerThread::run()
{
    while( Runnable* runnable = getCurrentRunnable() ) {
        if ( !mShuttingDown ) {
            runnable->runInternal();
        }
        dequeueCurrentRunnable();

        if ( runnable->deleteWhenDone() ) {
            delete runnable;
        }
    }
}

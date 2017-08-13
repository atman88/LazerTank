#include <iostream>
#include "workerthread.h"

WorkerThread::WorkerThread() : mThread(0), mShuttingDown(false)
{
}

void WorkerThread::doWork( Runnable* runnable )
{
    if ( !mShuttingDown ) {
        mPendingMutex.lock();

        if ( !mPending.size() && mThread && mThread->joinable() ) {
            std::thread* t = mThread;
            mThread = 0;

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
            mThread = 0;
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

    if ( mPending.size() > 0 ) {
        return mPending.front();
    }
    return 0;
}

void WorkerThread::dequeueCurrentRunnable()
{
    std::lock_guard<std::mutex> guard(mPendingMutex);

    mPending.pop_front();
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

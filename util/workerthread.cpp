#include "workerthread.h"

WorkerThread::WorkerThread() : mThread(0), mShuttingDown(false)
{
}

void WorkerThread::doWork( Runnable* runnable )
{
    if ( !mShuttingDown ) {
        std::lock_guard<std::mutex> guard(mPendingMutex);

        if ( !mPending.size() && mThread && mThread->joinable() ) {
            mThread->join();
            delete mThread;
            mThread = 0;
        }

        mPending.push_back( runnable );

        if ( !mThread ) {
            mThread = new std::thread( &WorkerThread::run, this );
        }
    }
}

void WorkerThread::shutdown()
{
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
            runnable->run();
        }
        dequeueCurrentRunnable();

        if ( runnable->mDeleteWhenDone ) {
            delete runnable;
        }
    }
}

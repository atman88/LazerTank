#include "workerthread.h"

WorkerThread::WorkerThread() : mShuttingDown(false)
{
}

void WorkerThread::doWork( Runnable* runnable )
{
    if ( !mShuttingDown ) {
        mPending.push_back( runnable );
        start( LowPriority );
    }
}

void WorkerThread::shutdown()
{
    mShuttingDown = true;
    wait(3000);
}

void WorkerThread::run()
{
    while( mPending.size() > 0 && !mShuttingDown) {
        Runnable* runnable = mPending.front();
        if ( !mShuttingDown ) {
            runnable->run();
        }
        mPending.pop_front();
        if ( runnable->mDeleteWhenDone ) {
            delete runnable;
        }
    }
}

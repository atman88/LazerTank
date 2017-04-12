#include "workerthread.h"

WorkerThread::WorkerThread()
{
}

void WorkerThread::doWork(Runnable* runnable)
{
    mPending.push_back( runnable );
    start( LowPriority );
}

void WorkerThread::run()
{
    while (mPending.size() > 0 ) {
       mPending.front()->run();
       mPending.pop_front();
    }
}

#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include <list>
#include <mutex>
#include <thread>
#include <csetjmp>
#include <memory>

/**
 * @brief Interface for user-defined tasks
 */
class Runnable
{
public:
    virtual ~Runnable() {}

    virtual void run() = 0;

    virtual void runInternal() = 0;

    virtual bool deleteWhenDone() = 0;
};

class BasicRunnable : public Runnable
{
public:
    BasicRunnable( bool deleteWhenDone = false ) : mDeleteWhenDone(deleteWhenDone)
    {
    }
    virtual ~BasicRunnable() {}

protected:
    virtual void runInternal()
    {
        run();
    }

    bool deleteWhenDone() override
    {
        return mDeleteWhenDone;
    }

private:
    bool mDeleteWhenDone;
};

/**
 * @brief A task with error handling hooks
 */
class ErrorableRunnable : public BasicRunnable
{
public:
    ErrorableRunnable( bool deleteWhenDone = false ) : BasicRunnable(deleteWhenDone), mLastError(0)
    {
    }
    virtual ~ErrorableRunnable() {}

    virtual void onError( int errorCode ) = 0;

    void error( int errorCode )
    {
        onError( mLastError );
        mLastError = errorCode;
        std::longjmp( mJmpBuf, errorCode );
    }

protected:
    void runInternal() override
    {
        if ( !setjmp( mJmpBuf ) ) {
            run();
        }
    }

private:
    std::jmp_buf mJmpBuf;
    int mLastError;
};

/**
 * @brief Utility which queues and serially runs tasks in the background
 * Note that this class uses the std thread (rather than Qt's thread APIs) in order to facilitate object creation free of
 * thread affinity. This allows background creation of QObject's which are subsequently moved to the target (application)
 * thread.
 */
class WorkerThread
{
public:
    WorkerThread();

    /**
     * @brief Queue a task for execution
     * @param runnable The task to queue
     */
    void doWork( Runnable* runnable );

    /**
     * @brief Queue a shared task for execution
     * @param sharedRunnable
     */
    void doWork( std::shared_ptr<Runnable> sharedRunnable );

    /**
     * @brief Inform this class that the app is shutting down
     */
    void shutdown();

    /**
     * @brief Forcibly return this thread to it's initial state
     */
    void purge();

private:
    Runnable* getCurrentRunnable();
    void dequeueCurrentRunnable();

    void run();
    std::mutex mPendingMutex;
    std::list<Runnable*> mPending;
    std::thread* mThread;
    bool mShuttingDown;
};

#endif // WORKERTHREAD_H

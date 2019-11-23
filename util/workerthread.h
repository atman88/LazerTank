#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include <list>
#include <mutex>
#include <thread>
#include <csetjmp>
#include <memory>
#include <cstring>

/**
 * @brief Interface for user-defined tasks
 */
class Runnable
{
public:
    virtual ~Runnable() {}

    /**
     * @brief User-suppled method performed in the background
     */
    virtual void run() = 0;

    /**
     * @brief Hook for wrapping additional run behavior
     */
    virtual void runInternal() = 0;

    /**
     * @brief Hook for specifying delete ownership
     * @return true to force deletion of this runnable after it is run by the WorkerThread
     */
    virtual bool deleteWhenDone() = 0;
};

/**
 * @brief A runnable with default hooks provided. Deletion of this runnable is managed by the caller.
 */
class BasicRunnable : public Runnable
{
public:
    BasicRunnable()
    {
    }

protected:
    virtual void runInternal()
    {
        run();
    }

    bool deleteWhenDone()
    {
        return false;
    }
};

/**
 * @brief A runnable which provides error handling hooks
 */
class ErrorableRunnable : public BasicRunnable
{
public:
    ErrorableRunnable() : mJmpBuf{}, mLastError(-1)
    {
    }
    virtual ~ErrorableRunnable() override {}

    /**
     * @brief User-supplied hook for error reporting, recovery etc
     * @param errorCode User-defined error identifier
     */
    virtual void onError( int errorCode ) = 0;

    /**
     * @brief Raise an error. This method terminates the running run method (I.e. it does not return)
     * @param errorCode User-defined error identifier
     */
    void __attribute((noreturn)) error( int errorCode )
    {
        onError( errorCode );
        mLastError = errorCode;
        std::longjmp( mJmpBuf, errorCode );
    }

    /**
     * @brief Query whether this runnable encountered an error during it's last run
     * @return true if errored
     */
    bool errored() const
    {
        return mLastError >= 0;
    }

    /**
     * @brief Retrieve the error code for the last error encountered
     * @return The last raised error code or -1 if an error has not occurred
     */
    int getLastError() const
    {
        return mLastError;
    }

protected:
    void runInternal() override
    {
        mLastError = -1;
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
     * @brief Queue a task for execution. Ownership of the runnable is retained by the caller.
     * @param runnable The task to queue
     */
    void doWork( Runnable* runnable );

    /**
     * @brief Queue a shared task for execution. This method is useful for facilitating auto deletion of the runnable.
     * @param sharedRunnable
     */
    void doWork( const std::shared_ptr<Runnable>& sharedRunnable );

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

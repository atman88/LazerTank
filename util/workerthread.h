#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include <list>
#include <QThread>

/**
 * @brief Interface for user-defined tasks
 */
class Runnable
{
public:
    virtual void run() = 0;
};

/**
 * @brief Utility which queues and serially runs tasks in the background
 */
class WorkerThread : public QThread
{
public:
    WorkerThread();

    /**
     * @brief Queue a task for execution
     * @param runnable The task to queue
     */
    void doWork( Runnable* runnable );

private:
    void run() override;
    std::list<Runnable*> mPending;
};

#endif // WORKERTHREAD_H

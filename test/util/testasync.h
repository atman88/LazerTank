#ifndef TESTASYNC_H
#define TESTASYNC_H

#include <QTime>
#include <QEventLoop>

/**
 * @brief Utility
 */
class TestAsync
{
public:
    TestAsync();

    /**
     * @brief The test expression which can be invoked repeatedly until met.
     * @return true if the condition is met otherwise false
     */
    virtual bool condition() = 0;

    /**
     * @brief initiate the test
     * @param timeout Maximum milliseconds to wait
     * @return The condition result
     */
    bool test( int timeout = 1000 );

    QTime getCreateTime() const;

private:
//    QEventLoop mLoop;
    QTime mCreateTime;
};

#endif // TESTASYNC_H

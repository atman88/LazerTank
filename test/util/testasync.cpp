#include <iostream>
#include <QApplication>
#include <QTimerEvent>
#include <QThread>

#include "testasync.h"

bool TestAsync::test( int timeout )
{
    mCreateTime.start();
    bool result = false;
    int elapsed = 0;
    for(;;) {
        QApplication::processEvents( QEventLoop::AllEvents, timeout-elapsed );
        elapsed = mCreateTime.elapsed();
        if ( condition() ) {
            result = true;
            break;
        }
        if ( elapsed >= timeout ) {
            break;
        }
        QThread::yieldCurrentThread();
    }
    std::cout << "TestAsync: result=" << result << " t=" << elapsed << std::endl;
    return result;
}

QTime TestAsync::getCreateTime() const
{
    return mCreateTime;
}

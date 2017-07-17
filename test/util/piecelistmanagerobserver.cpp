#include <iostream>
#include <QTest>
#include "piecelistmanagerobserver.h"
#include "model/piecelistmanager.h"

using namespace std;

PieceListManagerObserver::PieceListManagerObserver(PieceListManager& list, int maxAppendCount, int maxEraseCount)
    : QObject(0), mAppendCount(0), mEraseCount(0), mMaxAppendCount(maxAppendCount), mMaxEraseCount(maxEraseCount)
{
    if ( !QObject::connect( &list, &PieceListManager::appended, this, &PieceListManagerObserver::appended ) ) {
        std::cout << "*** PieceListManagerObserver: connect failed!" << std::endl;
    }
    QObject::connect( &list, &PieceListManager::erased,   this, &PieceListManagerObserver::erased   );
    QObject::connect( &list, &PieceListManager::changed,  this, &PieceListManagerObserver::changed  );
}

void PieceListManagerObserver::appended( ModelPoint point )
{
    cout << "MoveObserver: appended " << point.mCol << "," << point.mRow << endl;
    QVERIFY( ++mAppendCount <= mMaxAppendCount );
}

void PieceListManagerObserver::erased( ModelPoint point )
{
    cout << "MoveObserver: erased " << point.mCol << "," << point.mRow << endl;
    QVERIFY( ++mEraseCount <= mMaxEraseCount );
}

void PieceListManagerObserver::changed( ModelPoint point )
{
    cout << "MoveObserver: changed " << point.mCol << "," << point.mRow << endl;
}

void PieceListManagerObserver::printCounts()
{
    cout << "MoveObserver #append:" << mAppendCount << " #erase:" << mEraseCount << endl;
}

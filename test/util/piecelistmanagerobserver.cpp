#include <iostream>
#include <QTest>
#include "piecelistmanagerobserver.h"
#include "model/piecelistmanager.h"

using namespace std;

PieceListManagerObserver::PieceListManagerObserver( PieceListManager& list, int maxAddCount, int maxEraseCount )
    : QObject(&list), mAddCount(0), mEraseCount(0), mMaxAddCount(maxAddCount), mMaxEraseCount(maxEraseCount)
{
    if ( !QObject::connect( &list, &PieceListManager::added, this, &PieceListManagerObserver::added ) ) {
        std::cout << "*** PieceListManagerObserver: connect failed!" << std::endl;
    }
    QObject::connect( &list, &PieceListManager::erased,   this, &PieceListManagerObserver::erased   );
    QObject::connect( &list, &PieceListManager::changed,  this, &PieceListManagerObserver::changed  );
}

void PieceListManagerObserver::added( ModelPoint point )
{
    cout << "MoveObserver: added " << point.mCol << "," << point.mRow << endl;
    QVERIFY( ++mAddCount <= mMaxAddCount );
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
    cout << "MoveObserver #add:" << mAddCount << " #erase:" << mEraseCount << endl;
}

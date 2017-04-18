#include <iostream>
#include <QTest>
#include "piecelistmanagerobserver.h"
#include "model/piecelistmanager.h"

using namespace std;

PieceListManagerObserver::PieceListManagerObserver(PieceListManager& list, int maxAppendCount, int maxEraseCount)
    : QObject(0), mAppendCount(0), mEraseCount(0), mMaxAppendCount(maxAppendCount), mMaxEraseCount(maxEraseCount)
{
    QObject::connect( &list, &PieceListManager::appended, this, &PieceListManagerObserver::appended );
    QObject::connect( &list, &PieceListManager::erased,   this, &PieceListManagerObserver::erased   );
    QObject::connect( &list, &PieceListManager::changed,  this, &PieceListManagerObserver::changed  );
}

void PieceListManagerObserver::appended(int col, int row)
{
    cout << "MoveObserver: appended " << col << "," << row << endl;
    QVERIFY( ++mAppendCount <= mMaxAppendCount );
}

void PieceListManagerObserver::erased(int col, int row)
{
    cout << "MoveObserver: erased " << col << "," << row << endl;
    QVERIFY( ++mEraseCount <= mMaxEraseCount );
}

void PieceListManagerObserver::changed(int col, int row)
{
    cout << "MoveObserver: changed " << col << "," << row << endl;
}

void PieceListManagerObserver::printCounts()
{
    cout << "MoveObserver #append:" << mAppendCount << " #erase:" << mEraseCount << endl;
}

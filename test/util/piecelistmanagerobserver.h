#ifndef PIECELISTMANAGEROBSERVER_H
#define PIECELISTMANAGEROBSERVER_H

#include <QObject>

class PieceListManager;

#include "model/modelpoint.h"


class PieceListManagerObserver : public QObject
{
    Q_OBJECT
public:
    PieceListManagerObserver( PieceListManager& list, int maxAppendCount = 100, int maxEraseCount = 100 );
    ~PieceListManagerObserver()
    {
    }

public slots:
    void appended( ModelPoint point );
    void erased( ModelPoint point );
    void changed( ModelPoint point );

    void printCounts();

public:
    int mAppendCount;
    int mEraseCount;
    int mMaxAppendCount;
    int mMaxEraseCount;
};

#endif // PIECELISTMANAGEROBSERVER_H

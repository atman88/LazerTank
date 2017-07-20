#ifndef PIECELISTMANAGEROBSERVER_H
#define PIECELISTMANAGEROBSERVER_H

#include <QObject>

class PieceListManager;

#include "model/modelpoint.h"


class PieceListManagerObserver : public QObject
{
    Q_OBJECT
public:
    PieceListManagerObserver( PieceListManager& list, int maxAddCount = 100, int maxEraseCount = 100 );
    ~PieceListManagerObserver()
    {
    }

public slots:
    void added( ModelPoint point );
    void erased( ModelPoint point );
    void changed( ModelPoint point );

    void printCounts();

public:
    int mAddCount;
    int mEraseCount;
    int mMaxAddCount;
    int mMaxEraseCount;
};

#endif // PIECELISTMANAGEROBSERVER_H

#ifndef PIECELISTMANAGEROBSERVER_H
#define PIECELISTMANAGEROBSERVER_H

#include <QObject>

class PieceListManager;


class PieceListManagerObserver : public QObject
{
    Q_OBJECT
public:
    PieceListManagerObserver( PieceListManager& list, int maxAppendCount = 100, int maxEraseCount = 100 );
    ~PieceListManagerObserver()
    {
    }

public slots:
    void appended( int col, int row );
    void erased( int col, int row );
    void changed( int col, int row );

    void printCounts();

public:
    int mAppendCount;
    int mEraseCount;
    int mMaxAppendCount;
    int mMaxEraseCount;
};

#endif // PIECELISTMANAGEROBSERVER_H

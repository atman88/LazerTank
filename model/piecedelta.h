#ifndef PIECEDELTA_H
#define PIECEDELTA_H

#include <QObject>

#include "piecesetmanager.h"

class PieceDelta : public QObject
{
    Q_OBJECT

public:
    explicit PieceDelta(QObject *parent = 0);
    void init( PieceSetManager* masterManager, PieceSetManager* changesManager );
    bool update();
    const PieceSet* getPieces();

signals:
    void squareDirty( int x, int y );

private:
    void setDirtyFor( Piece* piece );

    PieceSetManager   mPieceManager;

    PieceSetManager*  mMasterManager;
    PieceSetManager*  mChangesManager;

    int mLastMasterTransactionNo;
    int mLastChangesTransactionNo;
};

#endif // PIECEDELTA_H

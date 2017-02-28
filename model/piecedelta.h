#ifndef PIECEDELTA_H
#define PIECEDELTA_H

#include <QObject>

#include "piecesetmanager.h"

class PieceDelta : public QObject
{
    Q_OBJECT

public:
    explicit PieceDelta(QObject *parent = 0);
    void init( const PieceSetManager* masterManager, PieceSetManager* changesManager );
    const PieceSetManager* getPieceManager() const;
    bool enabled() const;
    void enable( bool newValue = true );

public slots:
    void onChangeAt( int x, int y );

private:
    PieceSetManager   mPieceManager;

    const PieceSetManager*  mMasterManager;
    PieceSetManager*        mChangesManager;

    int mLastMasterTransactionNo;
    int mLastChangesTransactionNo;
    bool mEnabled;
};

#endif // PIECEDELTA_H

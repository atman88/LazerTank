#ifndef PIECEDELTA_H
#define PIECEDELTA_H

#include <QObject>

#include "board.h"
#include "piecesetmanager.h"

class PieceDelta : public QObject
{
    Q_OBJECT

public:
    explicit PieceDelta(QObject *parent = 0);
    void init( Board* masterBoard, Board* futureBoard );
    const PieceSetManager* getPieceManager() const;
    Board* getFutureBoard();
    bool enabled() const;
    void enable( bool newValue = true );

public slots:
    void onChangeAt( int x, int y );

private:
    PieceSetManager   mPieceManager;

    Board*  mMasterBoard;
    Board*  mFutureBoard;

    bool mEnabled;
};

#endif // PIECEDELTA_H

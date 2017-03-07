#ifndef TANK_H
#define TANK_H

#include <QObject>
#include <QPainter>
#include <QPropertyAnimation>

#include "controller/speedcontroller.h"
#include "view/tankview.h"
#include "model/piecelistmanager.h"

class Game;

class Tank : public TankView
{
    Q_OBJECT

public:
    Tank(QObject *parent = 0);
    virtual ~Tank() {}
    void init( Game* game );

    PieceListManager* getMoves();
    void move( int direction = -1 );

    int getCol() const;
    int getRow() const;

signals:
    void moved( int col, int row );
    void movingInto( int col, int row, int curRotation );
    void idled();

public slots:
    void reset( int col, int row );
    void onAnimationsFinished();

protected:
    void onMoved( int col, int row ) override;

private:
    void followPath();

    int mCol;
    int mRow;
    PieceListManager mMoves;
    bool mInReset;
};

#endif // TANK_H

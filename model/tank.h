#ifndef TANK_H
#define TANK_H

#include <QObject>
#include <QPainter>
#include <QPropertyAnimation>

#include "controller/speedcontroller.h"
#include "view/shooter.h"
#include "model/piecelistmanager.h"

class Game;

class Tank : public Shooter
{
    Q_OBJECT

public:
    Tank(QObject *parent = 0);
    virtual ~Tank() {}

    void init( Game* game );
    PieceListManager* getMoves();
    void render( const QRect* rect, QPainter* painter );
    void move( int direction = -1 );
    void pause();
    void resume();
    void stop();

    int getCol() const;
    int getRow() const;

signals:
    void changed( const QRect& rect );
    void moved( int col, int row );
    void movingInto( int col, int row, int curRotation );
    void idled();

public slots:
    void reset( int col, int row );
    void reset( QPoint& p );
    void onAnimationsFinished();
    void setX(const QVariant &x ) override;
    void setY(const QVariant &y ) override;
    void setRotation( const QVariant& angle ) override;

private:
    void followPath();

    QRect mPreviousPaintRect;
    RotateSpeedControlledAnimation mRotateAnimation;
    MoveSpeedControlledAnimation   mHorizontalAnimation;
    MoveSpeedControlledAnimation   mVerticalAnimation;

    PieceListManager mMoves;

    int mCol;
    int mRow;

    bool mInReset;
};

#endif // TANK_H

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
    void render( QPainter* painter );
    void move( int direction = -1 );
    void stop();

signals:
    void changed( const QRect& rect );
    void moved( int boardX, int boardY );
    void movingInto( int x, int y, int curRotation );
    void idled();

public slots:
    void reset( int boardX, int boardY );
    void reset( QPoint& p );
    void onAnimationsFinished();
    void setX(const QVariant &x ) override;
    void setY(const QVariant &y ) override;
    void setRotation( const QVariant& angle ) override;

private:
    void followPath();
    Game* getGame();

    QRect mPreviousPaintRect;
    RotateSpeedControlledAnimation mRotateAnimation;
    MoveSpeedControlledAnimation   mHorizontalAnimation;
    MoveSpeedControlledAnimation   mVerticalAnimation;

    PieceListManager mMoves;

    bool mInReset;
};

#endif // TANK_H

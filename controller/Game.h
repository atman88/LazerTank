#ifndef GAME_H
#define GAME_H

#include <QObject>

struct GameHandle;
class Game;

#include "animationaggregator.h"
#include "model/board.h"
#include "view/push.h"
#include "view/shot.h"
#include "view/shooter.h"
#include "view/BoardWindow.h"

struct GameHandle
{
    class Game* game;
};
Q_DECLARE_METATYPE(GameHandle)


class Game : public QObject
{
    Q_OBJECT

public:
    Game( Board* board );
    GameHandle getHandle();
    void init( BoardWindow* window );
    Board* getBoard();
    QAbstractAnimation::State animationState();
    Push& getMovingPiece();
    Shot& getCannonShot();
    AnimationAggregator* getMoveAggregate();
    AnimationAggregator* getShotAggregate();
    bool canMoveFrom(PieceType what, int angle, int *x, int *y , bool canPush = true);
    bool canShootFrom( int *angle, int *x, int *y );
    bool getAdjacentPosition( int angle, int *x, int *y );
    bool canPlaceAt(PieceType what, int x, int y , int fromAngle, bool canPush);
    bool canShootThru( int x, int y, int *angle );

public slots:
    void onTankMoved( int x, int y );
    void onBoardTileChanged( int x, int y );
    void onTankMovingInto( int x, int y, int fromAngle );

private:
    void sightCannons();

    AnimationAggregator mMoveAggregate;
    AnimationAggregator mShotAggregate;

    GameHandle mHandle;
    Board* mBoard;
    Push mMovingPiece;
    Shooter mActiveCannon;
    Shot mCannonShot;

    int mTankBoardX;
    int mTankBoardY;
};

#endif // GAME_H

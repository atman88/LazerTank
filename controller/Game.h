#ifndef GAME_H
#define GAME_H

#include <QObject>

struct GameHandle;
class Game;

#include "animationaggregator.h"
#include "pathfinder.h"
#include "model/piecedelta.h"
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
    SpeedController* getSpeedController();
    AnimationAggregator* getMoveAggregate();
    AnimationAggregator* getShotAggregate();
    bool canMoveFrom(PieceType what, int angle, int *x, int *y , bool futuristic, bool* pushResult = 0 );
    bool canShootFrom(int *angle, int *x, int *y , int *endOffset, Shot* source );
    bool getAdjacentPosition( int angle, int *x, int *y );
    bool canPlaceAtNonFuturistic(PieceType what, int x, int y , int fromAngle, bool *pushResult = 0);
    bool canShootThru( int x, int y, int *angle , int *endOffset, Shot* source );
    void onFuturePush(Piece *pushingPiece );
    void findPath(int fromX, int fromY, int targetX, int targetY, int targetRotation );
    const PieceSet* getDeltaPieces();
    void undoPush( Piece* pusher );

public slots:
    void onTankMoved( int x, int y );
    void onTankMovingInto( int x, int y, int fromAngle );
    void onBoardLoaded();
    void onBoardTileChanged( int x, int y );
    void onMovingPieceChanged(QAbstractAnimation::State newState, QAbstractAnimation::State oldState);
    void sightCannons();
    void endMoveDeltaTracking();

private:
    bool canMoveFrom(PieceType what, int angle, int *x, int *y, Board* board, bool *pushResult = 0 );
    bool canPlaceAt(PieceType what, int x, int y, int fromAngle, Board* board, bool *pushResult = 0);
    bool onShootThruMovingPiece( int offset, int angle, int *endOffset );

    SpeedController mSpeedController;
    AnimationAggregator mMoveAggregate;
    AnimationAggregator mShotAggregate;

    GameHandle mHandle;
    Board* mBoard;
    PathFinder mPathFinder;
    Push mMovingPiece;
    Shooter mActiveCannon;
    Shot mCannonShot;
    Board mFutureBoard;
    PieceDelta mFutureDelta;

    int mTankBoardX;
    int mTankBoardY;
};

#endif // GAME_H

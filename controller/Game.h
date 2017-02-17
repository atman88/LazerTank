#ifndef GAME_H
#define GAME_H

#include <QObject>

#include "animationaggregator.h"
#include "model/board.h"
#include "view/push.h"

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
    Board* getBoard();
    QAbstractAnimation::State animationState();
    Push& getMovingPiece();
    AnimationAggregator* getMoveAggregate();
    AnimationAggregator* getShotAggregate();
    bool canMoveFrom(PieceType what, int angle, int *x, int *y , bool canPush = true);
    bool canShootFrom( int *angle, int *x, int *y );
    bool getAdjacentPosition( int angle, int *x, int *y );
    bool canPlaceAt(PieceType what, int x, int y , int fromAngle, bool canPush);
    bool canShootThru( int x, int y, int *angle );

signals:
    void pieceAdded( const Piece& );
    void pieceRemoved( const Piece& );
    void rectDirty( const QRect& );
    void boardTileChanged( QRect rect );

public slots:
    void onTankMoved( int x, int y );
    void onBoardTileChanged( int x, int y );
    void onTankMovingInto( int x, int y, int fromAngle );

private:
    AnimationAggregator mMoveAggregate;
    AnimationAggregator mShotAggregate;

    GameHandle mHandle;
    Board* mBoard;
    Push mMovingPiece;
};

#endif // GAME_H

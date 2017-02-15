#ifndef GAME_H
#define GAME_H

#include <QObject>

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
    Push& getMovingPiece();
    bool canMoveFrom(PieceType what, int angle, int *x, int *y , bool canPush = true);
    bool canShootFrom( int angle, int *x, int *y );
    bool getAdjacentPosition( int angle, int *x, int *y );
    bool canPlaceAt(PieceType what, int x, int y );
    bool canShootThru(int angle, int x, int y );

signals:
    void pieceAdded( const Piece& );
    void pieceRemoved( const Piece& );
    void rectDirty( const QRect& );
    void boardTileChanged( QRect rect );

public slots:
    void onTankMoved( int x, int y );
    void onBoardTileChanged( int x, int y );

private:
    GameHandle mHandle;
    Board* mBoard;
    Push mMovingPiece;
};

#endif // GAME_H

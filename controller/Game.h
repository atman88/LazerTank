#ifndef GAME_H
#define GAME_H

#include <QObject>

#include "model/board.h"

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
    int getTankX();
    int getTankY();
    bool canMoveFrom( int angle, int *x, int *y );
    bool addMove( int angle );
    bool getAdjacentPosition( int angle, int *x, int *y );
    bool canPlaceAt( int x, int y );

signals:
    void pieceAdded( const Piece& );
    void pieceRemoved( const Piece& );
    void tankInitialized( int x, int y );

public slots:
    void clearPieces();
    void onTankMoved( int x, int y );

private:
    void addMoveInternal( int angle, int x, int y );
    GameHandle mHandle;
    Board* mBoard;
    int mTankX;
    int mTankY;
    PieceList mMoves;
};

#endif // GAME_H

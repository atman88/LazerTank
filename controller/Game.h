#ifndef GAME_H
#define GAME_H

#include <QObject>
#include <QPropertyAnimation>
#include <QRect>

#include "model/board.h"

struct GameHandle
{
    class Game* game;
};
Q_DECLARE_METATYPE(GameHandle)


class Game : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant pieceX READ getPieceX WRITE setPieceX)
    Q_PROPERTY(QVariant pieceY READ getPieceY WRITE setPieceY)

public:
    Game( Board* board );
    GameHandle getHandle();
    Board* getBoard();
    int getTankX();
    int getTankY();
    bool canMoveFrom(PieceType what, int angle, int *x, int *y , bool canPush = true);
    bool canShootFrom( int angle, int *x, int *y );
    bool addMove( int angle );
    bool getAdjacentPosition( int angle, int *x, int *y );
    bool canPlaceAt(PieceType what, int x, int y );
    bool canShootThru(int angle, int x, int y );

    QVariant getPieceX();
    QVariant getPieceY();
    PieceType getMovingPieceType();

signals:
    void pieceAdded( const Piece& );
    void pieceRemoved( const Piece& );
    void pieceMoved( const QRect& );
    void rectDirty( const QRect );
    void pieceStopped();
    void tankInitialized( int x, int y );
    void boardTileChanged( QRect rect );

public slots:
    void clearMoves();
    void onTankMoved( int x, int y );
    void onBoardLoaded();
    void onBoardTileChanged( int x, int y );
    void setPieceX( const QVariant& x );
    void setPieceY( const QVariant& y );
    void onPieceStopped();

private:
    void addMoveInternal( int angle, int x, int y );
    GameHandle mHandle;
    Board* mBoard;
    int mTankX;
    int mTankY;
    PieceList mMoves;

    QPropertyAnimation* mHorizontalPieceAnimation;
    QPropertyAnimation* mVerticalPieceAnimation;
    QRect mPieceBoundingRect;
    PieceType mMovingPieceType;
};

#endif // GAME_H

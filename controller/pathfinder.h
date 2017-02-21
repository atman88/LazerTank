#ifndef PATHFINDER_H
#define PATHFINDER_H

class Game;
class PathFinder;

#include <csetjmp>
#include <QThread>
#include "model/board.h"
#include "model/piece.h"

#define MAX_POINTS (((BOARD_MAX_HEIGHT>BOARD_MAX_WIDTH) ? BOARD_MAX_HEIGHT : BOARD_MAX_WIDTH) * 2)

class PathFinder : public QThread
{
    Q_OBJECT

public:
    PathFinder( QObject *parent = 0 );
    void findPath(int fromX, int fromY, int targetX, int targetY, int targetRotation );
    void run() override;

signals:
    void pathFound( PieceList path );

private:
    void tryAt(int x, int y);
    int pass1( int nPoints );
    int pass2( int nPoints );
    void buildPath( int x, int y );
    Game* getGame();
    void printSearchMap();

    int mFromX,   mFromY;
    int mTargetX, mTargetY, mTargetRotation;
    bool mStopping;
    char mSearchMap[BOARD_MAX_HEIGHT*BOARD_MAX_WIDTH];
    int mMapWidth;
    int mMapHeight;
    int mSearchX[MAX_POINTS];
    int mSearchY[MAX_POINTS];
    int mPassValue;
    int mPushIndex;
    int mPushDirection;

    PieceList mMoves;

    std::jmp_buf mJmpBuf;
};

#endif // PATHFINDER_H

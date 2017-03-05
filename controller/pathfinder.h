#ifndef PATHFINDER_H
#define PATHFINDER_H

class Game;
class PathFinder;

#include <csetjmp>
#include <QThread>
#include "model/board.h"
#include "model/piecelistmanager.h"

// The maximum number of points to track for a given search pass. (A pass can consider nearly two rows worth)
#define MAX_POINTS (((BOARD_MAX_HEIGHT>BOARD_MAX_WIDTH) ? BOARD_MAX_HEIGHT : BOARD_MAX_WIDTH) * 2)

/**
 * @brief Computes a list of moves between two points for the current board.
 */
class PathFinder : public QThread
{
    Q_OBJECT

public:
    PathFinder( QObject *parent = 0 );

    /**
     * @brief Initiate a path search. If found, the resulting path is posted via the pathFound signal.
     * @param targetX The row of the starting square
     * @param targetY The column of the starting square
     * @param startingX The row of the destination square
     * @param startingY The column of the destination square
     * @param startingRotation The starting direction of the search
     */
    void findPath(int targetX, int targetY, int startingX, int startingY, int targetRotation );

    void run() override;

signals:
    void pathFound( PieceListManager* path );

private:
    void tryAt(int x, int y);
    int pass1( int nPoints );
    int pass2( int nPoints );
    void buildPath( int x, int y );
    Game* getGame();
    void printSearchMap();

    int mTargetX,   mTargetY;
    int mStartingX, mStartingY, mStartingRotation;
    bool mStopping;
    char mSearchMap[BOARD_MAX_HEIGHT*BOARD_MAX_WIDTH];
    int mMaxX;
    int mMaxY;
    int mSearchX[MAX_POINTS];
    int mSearchY[MAX_POINTS];
    int mPassValue;
    int mPushIndex;
    int mPushDirection;

    PieceListManager mMoves;

    std::jmp_buf mJmpBuf;
};

#endif // PATHFINDER_H

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
     * @param targetCol The row of the destination square
     * @param targetRow The column of the destination square
     * @param startCol The row of the starting square
     * @param startRow The column of the starting square
     * @param startRotation The starting direction of the search
     * @param testOnly If true, only the pathTestResult signal is raised as a result otherwise a pathFound signal
     * is raised if (and only if) the path is successful
     */
    void findPath(int targetCol, int targetRow, int startCol, int startRow, int startRotation, bool testOnly );

    void run() override;

signals:
    /**
     * @brief Notification of successful computed path results
     * @param targetCol the target column used in the search
     * @param targetRow the target row used for the search
     * @param startCol the starting column used for the search
     * @param startRow the starting row used for the search
     * @param startRotation the starting direction angle used for the search
     * @param path The result of the find
     */
    void pathFound( int targetCol, int targetRow, int startCol, int startRow, int startRotation, PieceListManager* path );

    /**
     * @brief Notification of a test result
     * @param reachable true if a path between the two points is possible, otherwise false
     * @param targetCol the target column used in the test
     * @param targetRow the target row used for the test
     * @param startCol the starting column used for the test
     * @param startRow the starting row used for the test
     * @param startRotation the starting direction angle used for the test
     */
    void testResult( bool reachable, int targetCol, int targetRow, int startCol, int startRow, int startRotation );

private:
    void tryAt(int col, int row);
    int pass1( int nPoints );
    int pass2( int nPoints );
    void buildPath( int col, int row );

    int mTargetCol, mTargetRow;
    int mStartCol, mStartRow, mStartRotation;
    bool mStopping;
    char mSearchMap[BOARD_MAX_HEIGHT*BOARD_MAX_WIDTH];
    int mMaxCol;
    int mMaxRow;
    int mSearchCol[MAX_POINTS];
    int mSearchRow[MAX_POINTS];
    int mPassValue;
    int mPushIndex;
    int mPushDirection;
    bool mTestOnly;

    PieceListManager mMoves;

    std::jmp_buf mJmpBuf;
};

#endif // PATHFINDER_H

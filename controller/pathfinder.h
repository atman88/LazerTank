#ifndef PATHFINDER_H
#define PATHFINDER_H

#include <memory>
#include <csetjmp>
#include <QThread>

class Game;
class PathFinder;

#include "model/board.h"
#include "model/piecelistmanager.h"
#include "pathsearchcriteria.h"

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
     * @param action The search criteria
     * @param testOnly If true, only the pathTestResult signal is raised as a result otherwise a pathFound signal
     * is raised if (and only if) the path is successful
     * @return true if the search is successfully started
     */
    bool findPath( PathSearchCriteria* criteria, bool testOnly );

    void run() override;

signals:
    /**
     * @brief Notification of successful computed path results
     * @param criteria The search parameters used for the search
     * @param path The result of the search
     */
    void pathFound( PathSearchCriteria criteria, PieceListManager* path );

    /**
     * @brief Notification of a test result
     * @param reachable true if a path between the two points is possible, otherwise false
     * @param criteria The search parameters that were tested
     */
    void testResult( bool reachable, PathSearchCriteria criteria );

private:
    void tryAt(int col, int row);
    int pass1( int nPoints );
    int pass2( int nPoints );
    void buildPath( int col, int row );

    PathSearchCriteria mCriteria;
    PathSearchCriteria mRunCriteria; // copy used by the background which won't be impacted by a parallel call to findPath
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

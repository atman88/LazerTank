#ifndef PATHFINDER_H
#define PATHFINDER_H

#include <csetjmp>

class Game;
class Push;
class PathFinder;

#include "model/board.h"
#include "model/piecelistmanager.h"
#include "pathsearchcriteria.h"
#include "util/workerthread.h"

// The maximum number of points to track for a given search pass. (A pass can consider nearly two rows worth)
#define MAX_POINTS (((BOARD_MAX_HEIGHT>BOARD_MAX_WIDTH) ? BOARD_MAX_HEIGHT : BOARD_MAX_WIDTH) * 2)

/**
 * @brief Computes a list of moves between two points for the current board.
 */
class PathFinder : public QObject
{
    Q_OBJECT

public:
    PathFinder( QObject* parent = nullptr );

    /**
     * @brief Initiate a search or test
     * @param criteria Parameters for the search or test
     * @param testOnly If true, the pathTestResult signal delivers the test result, otherwise the pathFound signal
     * is raised if (and only if) the path is successful
     * @return true if the search is successfully started
     */
    bool execCriteria( PathSearchCriteria* criteria, bool testOnly = true );

    /**
     * @brief Construct a path to the given target using the residue of a previous tile drag test
     * @param target The final endpoint vector of the resultant path
     * @return true if successful. To succeed, the preceeding execCriteria call must have been a TileDragTestCriteria
     * which included the given target.
     */
    bool buildTilePushPath( const ModelVector& target );

signals:
    /**
     * @brief Notification of successful computed path results
     * @param criteria The search parameters used for the search
     * @param path The result of the search
     */
    void pathFound( PathSearchCriteria criteria, PieceListManager* path );

    /**
     * @brief Notification of a test result
     * @param reachable true if a path between the start and any target points are possible, otherwise false
     * @param criteria The search parameters that were tested
     */
    void testResult( bool reachable, PathSearchCriteria criteria );

private:
    void doSearchInternal();
    void buildTilePushPathInternal( const ModelVector& target );
    void addPush( Push& push );
    bool tryAt( int col, int row );
    void pass1();
    void pass2();
    bool buildPath();

    PathSearchCriteria mCriteria;
    PathSearchCriteria mRunCriteria; // copy used by the background which won't be impacted by a parallel call to findPath
    bool mStopping;
    char mSearchMap[BOARD_MAX_HEIGHT*BOARD_MAX_WIDTH];
    ModelPoint mMaxPoint;
    int mSearchCol[MAX_POINTS];
    int mSearchRow[MAX_POINTS];
    unsigned mNPoints;
    int mPassValue;
    int mPushIndex;
    int mPushDirection;
    bool mTestOnly;
    std::set<ModelPoint> mTargets;

    PieceListManager mMoves;

    std::jmp_buf mJmpBuf;

    class PathSearchRunnable : public BasicRunnable
    {
    public:
        PathSearchRunnable( PathFinder& pathFinder ) : mPathFinder(pathFinder) {}

        void run() override;

        PathFinder& mPathFinder;
    } mPathSearchRunnable;

    class TileDragBuildRunnable : public BasicRunnable
    {
    public:
        TileDragBuildRunnable( PathFinder& pathFinder ) : mPathFinder(pathFinder) {}

        void run() override
        {
            mPathFinder.buildTilePushPathInternal( mTarget );
        }

        PathFinder& mPathFinder;
        ModelVector mTarget;
    } mTileDragBuildRunnable;

    friend class PathSearchRunnable;
    friend class TileDragBuildRunnable;
};

#endif // PATHFINDER_H

#ifndef PATHFINDERCONTROLLER_H
#define PATHFINDERCONTROLLER_H

#include <memory>
#include <QObject>

class Game;
class PathSearchAction;

#include "pathfinder.h"

/**
 * @brief An object providing controlled access to a path finder
 */
class PathFinderController : public QObject
{
    Q_OBJECT
public:
    explicit PathFinderController(QObject *parent = 0);
    void init( Game* game );

    /**
     * @brief Perform a search
     * @param action A search term container shared between the path finder and the ui
     */
    void doAction( std::shared_ptr<PathSearchAction> action );

    /**
     * @brief test multiple actions.
     * This method inividually enables/disables each action as per the result of its test.
     * @param actions An array of search term containers
     * @param count The dimension of the array
     */
    void testActions( std::shared_ptr<PathSearchAction> actions[], unsigned count );

signals:
    /**
     * @brief Notification of an action result
     * @param path
     * @param wakeup hints whether move animations should resume when applying this result.
     * If false, move animations are intended to resume on a subsequent trigger.
     */
    void pathFound( PieceListManager* path, bool wakeup );

public slots:
    /**
     * @brief Slots interfacing this controller with its underlying path finder
     */
    void onResult( bool succeeded, int targetCol, int targetRow, int startingCol, int startingRow, int targetRotation );
    void onPath( int targetCol, int targetRow, int startCol, int startRow, int targetRotation, PieceListManager* path );

private:
    /**
     * @brief Helper method to perform either an action or a test
     * @param action
     * @param testOnly true if the action should simply be tested, otherwise the action is performed
     */
    void doAction( std::shared_ptr<PathSearchAction> action, bool testOnly );

    /**
     * @brief test the next pending action if any
     */
    void testNextAction();

    PathFinder mPathFinder;
    bool mTestOnly;
    bool mMoveWhenFound;

    int mTargetCol, mTargetRow;
    int mStartCol, mStartRow, mStartDirection;

    std::list<std::shared_ptr<PathSearchAction>> mTestActions;
};

#endif // PATHFINDERCONTROLLER_H

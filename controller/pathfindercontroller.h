#ifndef PATHFINDERCONTROLLER_H
#define PATHFINDERCONTROLLER_H

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
    void init();

    /**
     * @brief Perform a search
     * @param action A search term container shared between the path finder and the ui
     * @param testOnly if true, the action is merely tested, otherwise the action is
     * performed and if successful, notifies the result via the pathFound signal.
     * @return true if the action is successfully started
     */
    bool doAction( PathSearchAction* action, bool testOnly = false );

    /**
     * @brief test multiple actions.
     * This method inividually enables/disables each action as per the result of its test.
     * @param actions An array of search term containers
     * @param count The dimension of the array
     */
    void testActions( PathSearchAction* actions[], unsigned count );

signals:
    /**
     * @brief Notification of an action result
     * @param path
     * @param action The search action that started the search
     * If false, move animations are intended to resume on a subsequent trigger.
     */
    void pathFound( PieceListManager* path, PathSearchAction* action );

protected slots:
    /**
     * @brief Slots interfacing this controller with its underlying path finder
     */
    void onResult( bool succeeded, PathSearchCriteria criteria );
    void onPath( PathSearchCriteria criteria, PieceListManager* path );

private:
    /**
     * @brief test the next pending action if any
     */
    void testNextAction();

    PathFinder mPathFinder;
    PathSearchAction* mCurAction;
    std::list<PathSearchAction*> mTestActions;
};

#endif // PATHFINDERCONTROLLER_H

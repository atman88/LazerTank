#ifndef PATHFINDERCONTROLLER_H
#define PATHFINDERCONTROLLER_H

#include <memory>
#include <QObject>

class Game;
class PathSearchAction;

#include "pathfinder.h"

class PathFinderController : public QObject
{
    Q_OBJECT
public:
    explicit PathFinderController(QObject *parent = 0);

    void init( Game* game );

    void doAction( std::shared_ptr<PathSearchAction> action );

    void testActions( std::shared_ptr<PathSearchAction> actions[], unsigned count );

signals:
    void pathFound( PieceListManager* path );

public slots:
    void onResult( bool succeeded, int targetCol, int targetRow, int startingCol, int startingRow, int targetRotation );
    void onPath( int targetCol, int targetRow, int startCol, int startRow, int targetRotation, PieceListManager* path );

private:
    void doAction( std::shared_ptr<PathSearchAction> action, bool testOnly );
    void testNextAction();

    PathFinder mPathFinder;
    bool mTestOnly;
    bool mMoveWhenFound;

    int mTargetCol, mTargetRow;
    int mStartCol, mStartRow, mStartDirection;

    std::list<std::weak_ptr<PathSearchAction>> mTestActions;
};

#endif // PATHFINDERCONTROLLER_H

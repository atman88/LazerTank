#ifndef GAMEREGISTRY_H
#define GAMEREGISTRY_H

#include <QObject>
#include "pathsearchaction.h"
#include "util/gameutils.h"

struct GameHandle;
class Game;
class BoardWindow;
class WorkerThread;

class GameRegistry : public QObject
{
    Q_OBJECT
public:
    explicit GameRegistry( Game& game, BoardWindow* window, WorkerThread& worker );
    ~GameRegistry();
    GameHandle getHandle();

signals:

private slots:
    void onWindowDestroyed();

public:
    GameHandle mHandle;
    Game& mGame;
    BoardWindow* mWindow;
    WorkerThread& mWorker;
    PathSearchAction mCaptureAction;
    PathSearchAction mPathToAction;
};

#endif // GAMEREGISTRY_H

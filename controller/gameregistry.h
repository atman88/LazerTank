#ifndef GAMEREGISTRY_H
#define GAMEREGISTRY_H

#include <QObject>
#include "pathsearchaction.h"
#include "util/gameutils.h"
#include "util/workerthread.h"

struct GameHandle;
class Game;
class BoardWindow;
class SpeedController;
class MoveController;
class PathFinderController;
class AnimationStateAggregator;
class BoardPool;
class LevelList;
class Tank;
class Shooter;
class ShotModel;
class Push;
class Recorder;
class Persist;

class GameRegistry : public QObject
{
    Q_OBJECT
public:
    explicit GameRegistry( BoardWindow* window = 0 );
    ~GameRegistry();
    GameHandle getHandle();

    /**
     * @brief Access the current laser shot being fired
     * Only one cannon is fired at a time. This is the single lazer path shot from any single selected cannon
     */
    ShotModel& getCannonShot();

    /**
     * @brief Access the Game engine
     */
    Game& getGame();

    /**
     * @brief Get the main window
     * @return The window or 0 if not registered
     */
    BoardWindow* getWindow();

    /**
     * @brief Access the game animation speed manager
     */
    SpeedController& getSpeedController();

    /**
     * @brief Access the tank manager
     */
    MoveController& getMoveController();

    /**
     * @brief Get Managed access to the path finder
     */
    PathFinderController& getPathFinderController();

    /**
     * @brief Get the aggregate state tracker for the current tank move including any push the move is doing
     */
    AnimationStateAggregator& getMoveAggregate();

    /**
     * @brief Get the aggregate state tracker for any active shots including any pushes the active shots are doing
     */
    AnimationStateAggregator& getShotAggregate();

    /**
     * @brief The pool of loaded boards
     */
    BoardPool& getBoardPool();

    /**
     * @brief The game's one-and-only tank
     */
    Tank& getTank();

    /**
     * @brief Access the active cannon container
     */
    Shooter& getActiveCannon();

    /**
     * @brief Access the container for the piece currently being pushed by the tank
     * It's type is NONE when the tank isn't pushing a piece
     */
    Push& getTankPush();

    /**
     * @brief Access the container for the piece currently being pushed by a shot
     * It's type is NONE when no piece is being shot
     */
    Push& getShotPush();

    /**
     * @brief Access the list of available levels
     */
    LevelList& getLevelList();

    /**
     * @brief Access the game recorder
     */
    Recorder& getRecorder();

    /**
     * @brief Get the persistent storage utility
     */
    Persist& getPersist();

    /**
     * @brief Access to the background thread
     */
    WorkerThread& getWorker();

    /**
     * @brief Get the flag capture action container
     */
    PathSearchAction& getCaptureAction();

    /**
     * @brief Get the path action container
     */
    PathSearchAction& getPathToAction();

private slots:
    void onWindowDestroyed();

protected:
    GameHandle mHandle;
    BoardWindow* mWindow;
    Game* mGame;
    SpeedController* mSpeedController;
    MoveController* mMoveController;
    PathFinderController* mPathFinderController;
    AnimationStateAggregator* mMoveAggregate;
    AnimationStateAggregator* mShotAggregate;
    BoardPool* mBoardPool;
    Tank* mTank;
    Shooter* mActiveCannon;
    Push* mTankPush;
    Push* mShotPush;
    LevelList* mLevelList;
    Recorder* mRecorder;
    Persist* mPersist;

    WorkerThread mWorker;
    PathSearchAction mCaptureAction;
    PathSearchAction mPathToAction;
};

#endif // GAMEREGISTRY_H

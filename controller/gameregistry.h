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
class LevelChooser;
class Tank;
class Shooter;
class ShotModel;
class Push;

class GameRegistry : public QObject
{
    Q_OBJECT
public:
    explicit GameRegistry( BoardWindow* window = 0, Game* game = 0,
      SpeedController* speedController = 0, MoveController* moveController = 0, PathFinderController* pathFinderController = 0,
      AnimationStateAggregator* moveAggregate = 0, AnimationStateAggregator* shotAggregate = 0,
      Tank* tank = 0, Shooter* activeCannon = 0, Push* tankPush = 0, Push* shotPush = 0, LevelChooser* levelChooser = 0 );
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
    LevelChooser& getLevelChooser();

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

private:
    void onInject( QObject* object );

    GameHandle mHandle;
    BoardWindow* mWindow;
    Game* mGame;
    SpeedController* mSpeedController;
    MoveController* mMoveController;
    PathFinderController* mPathFinderController;
    AnimationStateAggregator* mMoveAggregate;
    AnimationStateAggregator* mShotAggregate;
    Tank* mTank;
    Shooter* mActiveCannon;
    Push* mTankPush;
    Push* mShotPush;
    LevelChooser* mLevelChooser;

    WorkerThread mWorker;
    PathSearchAction mCaptureAction;
    PathSearchAction mPathToAction;

    QObjectList mInjectionList;
};

#endif // GAMEREGISTRY_H

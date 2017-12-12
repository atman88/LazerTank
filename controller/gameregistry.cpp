#include <QVariant>
#include "gameregistry.h"
#include "view/boardwindow.h"
#include "controller/game.h"
#include "controller/speedcontroller.h"
#include "controller/movecontroller.h"
#include "controller/pathfindercontroller.h"
#include "controller/animationstateaggregator.h"
#include "model/boardpool.h"
#include "model/tank.h"
#include "view/shooter.h"
#include "view/levelchooser.h"
#include "model/push.h"
#include "util/workerthread.h"
#include "util/recorder.h"
#include "util/persist.h"

GameRegistry::GameRegistry( BoardWindow* window ) : QObject(0), mWindow(window)
#define DECL_NULL_INIT(o) ,m##o(0)
  DECL_NULL_INIT(Game)
  DECL_NULL_INIT(SpeedController)
  DECL_NULL_INIT(MoveController)
  DECL_NULL_INIT(PathFinderController)
  DECL_NULL_INIT(MoveAggregate)
  DECL_NULL_INIT(ShotAggregate)
  DECL_NULL_INIT(BoardPool)
  DECL_NULL_INIT(Tank)
  DECL_NULL_INIT(ActiveCannon)
  DECL_NULL_INIT(TankPush)
  DECL_NULL_INIT(ShotPush)
  DECL_NULL_INIT(LevelList)
  DECL_NULL_INIT(Recorder)
  DECL_NULL_INIT(Persist)
{
    mHandle.registry = this;
    setProperty( GameHandleName, QVariant::fromValue(mHandle) );

    mCaptureAction.setParent(this);
    mPathToAction.setParent(this);
    mPathToAction.setText( "Move &Here" );

    if ( window ) {
        QObject::connect( window, &BoardWindow::destroyed, this, &GameRegistry::onWindowDestroyed, Qt::DirectConnection );
        window->setProperty( GameHandleName, property(GameHandleName) );
        mWindow = window;
    }
}

void GameRegistry::onWindowDestroyed()
{
    mWindow = 0;
}

BoardWindow*GameRegistry::getWindow()
{
    return mWindow;
}

ShotModel& GameRegistry::getCannonShot()
{
    return getActiveCannon().getShot();
}

#define DECL_GETTER(name,type) \
    type& GameRegistry::get##name() { if ( !m##name ) { m##name = new type(); m##name->setParent(this); } return *m##name; }

DECL_GETTER(Game,Game)
DECL_GETTER(SpeedController,SpeedController)
DECL_GETTER(MoveController,MoveController)
DECL_GETTER(PathFinderController,PathFinderController)
DECL_GETTER(MoveAggregate,AnimationStateAggregator)
DECL_GETTER(ShotAggregate,AnimationStateAggregator)
DECL_GETTER(BoardPool,BoardPool)
DECL_GETTER(Tank,Tank)
DECL_GETTER(ActiveCannon,Shooter)
DECL_GETTER(TankPush,Push)
DECL_GETTER(ShotPush,Push)
DECL_GETTER(LevelList,LevelList)
DECL_GETTER(Recorder,Recorder)
DECL_GETTER(Persist,Persist)

WorkerThread&     GameRegistry::getWorker()        { return mWorker;        }
PathSearchAction& GameRegistry::getCaptureAction() { return mCaptureAction; }
PathSearchAction& GameRegistry::getPathToAction()  { return mPathToAction;  }

GameRegistry::~GameRegistry()
{
    mWorker.shutdown();
}

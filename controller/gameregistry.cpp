#include <QVariant>
#include "gameregistry.h"
#include "view/boardwindow.h"
#include "controller/game.h"
#include "controller/speedcontroller.h"
#include "controller/movecontroller.h"
#include "controller/pathfindercontroller.h"
#include "controller/animationstateaggregator.h"
#include "model/tank.h"
#include "view/shooter.h"
#include "model/push.h"
#include "util/workerthread.h"

GameRegistry::GameRegistry( BoardWindow* window, Game* game,
  SpeedController* speedController, MoveController* moveController, PathFinderController* pathFinderController,
  AnimationStateAggregator* moveAggregate, AnimationStateAggregator* shotAggregate, Tank* tank, Shooter* activeCannon,
  Push* tankPush, Push* shotPush )
  : QObject(0), mWindow(window), mGame(game),
    mSpeedController(speedController), mMoveController(moveController), mPathFinderController(pathFinderController),
    mMoveAggregate(moveAggregate), mShotAggregate(shotAggregate), mTank(tank), mActiveCannon(activeCannon),
    mTankPush(tankPush), mShotPush(shotPush)
{
    mHandle.registry = this;
    setProperty("GameHandle", QVariant::fromValue(mHandle));

    if ( game                  ) onInject( game );
    if ( speedController       ) onInject( speedController );
    if ( moveController        ) onInject( moveController );
    if ( pathFinderController  ) onInject( pathFinderController );
    if ( moveAggregate         ) onInject( moveAggregate );
    if ( shotAggregate         ) onInject( shotAggregate );
    if ( activeCannon          ) onInject( activeCannon );
    if ( tankPush              ) onInject( tankPush );
    if ( shotPush              ) onInject( shotPush );

    mWorker.setParent(this);
    mCaptureAction.setParent(this);
    mPathToAction.setParent(this);

    if ( window ) {
        QObject::connect( window, &BoardWindow::destroyed, this, &GameRegistry::onWindowDestroyed );
    }
}

ShotModel& GameRegistry::getCannonShot()
{
    return getActiveCannon().getShot();
}

void GameRegistry::onWindowDestroyed()
{
    mWindow = 0;
}

BoardWindow*GameRegistry::getWindow()
{
    return mWindow;
}

#define DECL_GETTER(name,type) \
    type& GameRegistry::get##name() { if ( !m##name ) { m##name = new type(); m##name->setParent(this); } return *m##name; }

DECL_GETTER(Game,Game)
DECL_GETTER(SpeedController,SpeedController)
DECL_GETTER(MoveController,MoveController)
DECL_GETTER(PathFinderController,PathFinderController)
DECL_GETTER(MoveAggregate,AnimationStateAggregator)
DECL_GETTER(ShotAggregate,AnimationStateAggregator)
DECL_GETTER(Tank,Tank)
DECL_GETTER(ActiveCannon,Shooter)
DECL_GETTER(TankPush,Push)
DECL_GETTER(ShotPush,Push)

WorkerThread&     GameRegistry::getWorker()        { return mWorker;        }
PathSearchAction& GameRegistry::getCaptureAction() { return mCaptureAction; }
PathSearchAction& GameRegistry::getPathToAction()  { return mPathToAction;  }

void GameRegistry::onInject( QObject* object )
{
    mInjectionList.append( object );
    object->setParent( this );
}

GameRegistry::~GameRegistry()
{
    // for injection support, disassociate the children of this class to inhibit deleting
    for( QObject* object : mInjectionList ) {
        object->setParent(0);
    }
}


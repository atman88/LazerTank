#ifndef TESTMAIN_H
#define TESTMAIN_H

#include <QObject>
#include <QTest>
#include <QSignalSpy>

class TestPathFinderController;

#include "controller/gameregistry.h"
#include "controller/game.h"
#include "controller/speedcontroller.h"
#include "controller/movecontroller.h"
#include "controller/pathfindercontroller.h"
#include "controller/animationstateaggregator.h"
#include "model/boardpool.h"
#include "model/tank.h"
#include "view/shooter.h"
#include "model/push.h"
#include "model/level.h"
#include "util/recorder.h"
#include "util/persist.h"

class TestRegistry : public GameRegistry
{
public:
    TestRegistry() = default;
    ~TestRegistry();

    /**
     * @brief reset the registry (used to cleanup after each test is run)
     */
    void cleanup();

#define DECL_INJECT(name,type)\
    void inject##name(type* p##name) {\
      QVERIFY(m##name == nullptr);\
      m##name=p##name;\
      p##name->setParent(this);\
    }

DECL_INJECT(Game,Game)
DECL_INJECT(SpeedController,SpeedController)
DECL_INJECT(MoveController,MoveController)
DECL_INJECT(PathFinderController,PathFinderController)
DECL_INJECT(MoveAggregate,AnimationStateAggregator)
DECL_INJECT(ShotAggregate,AnimationStateAggregator)
DECL_INJECT(BoardPool,BoardPool)
DECL_INJECT(Tank,Tank)
DECL_INJECT(ActiveCannon,Shooter)
DECL_INJECT(TankPush,Push)
DECL_INJECT(ShotPush,Push)
DECL_INJECT(LevelList,LevelList)
DECL_INJECT(Recorder,Recorder)
DECL_INJECT(Persist,Persist)
};

class TestMain : public QObject
{
    Q_OBJECT
public:
    TestMain();
    ~TestMain();

    void initGame( const char* map );
    void initGame( QTextStream& map );

    TestPathFinderController* setupTestDrag( const char* map );
    Persist* setupTestPersist();

    TestRegistry* getRegistry();

private slots:
    void testIsMasterBoard();

    void testMeasureShot();
    void testCannonShot();

    void testFutureSelect();
    void testDragTile();
    void testDragTank();
    void testDragPoint();
    void testDragWithMove();

    void testPersistSizes();
    void testPersistNew();
    void testPersistReplace();

    void testLevelFind();
    void testNextLevel();
    void testLevelCompleted();

    void testBoardPool();

    void testGameMove();
    void testGameCannon();
    void testGamePush();

    void testPieceListManager();

    void testFutureShotPath();
    void testFutureShotThruStationaryTank();
    void testFutureShotThruMasterTank();
    void testFutureMultiShotThruTank();
    void testFutureShotPushId();
    void testFutureShotPartialUndo();
    void testFutureShotPushIdWater();
    void testFutureShot2PushIdWater();
    void testFutureShotTankKill();

    void testRecorderBitFields();
    void testRecorderRecordSize();
    void testRecorderOverflow();

    void testFocus();
    void testMultiShotQueued();
    void testMultiShotShotDirty();
    void testMultiShotShooterRelease();
    void testMultiShotShotFinished();
    void testReplay();
    void testMoveFocus();

    void testWorker();

    void cleanup();

private:
    QTextStream* mStream;
    TestRegistry mRegistry;
};

/**
 * @brief Wrapper for the dirty signal to allow its passed-by-reference signal arg
 */
class DirtySpy : public QObject
{
    Q_OBJECT

public:
    DirtySpy(QObject* object );
    ~DirtySpy();
    bool wait( int msecs = 5000 );

public slots:
    void rectDirty(QRect& rect);
signals:
    void dirty();

private:
    QSignalSpy* mSignalSpy;
};

class SignalReceptor : public QObject
{
    Q_OBJECT

public:
    SignalReceptor() : QObject(nullptr), mReceived(false)
    {
    }

    SignalReceptor( const QObject* sender, const char* signal ) : QObject(nullptr), mReceived(false)
    {
        QObject::connect( sender, signal, this, SLOT(receive()) );
    }

    ~SignalReceptor()
    {
        QObject::disconnect( this );
    }

    bool mReceived;

public slots:
    void receive() { mReceived = true; }
};

#endif // TESTMAIN_H

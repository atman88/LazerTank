#include <iostream>
#include "../testmain.h"
#include "../test/util/testasync.h"
#include "shotmodel.h"

using namespace std;

class TestShotModel : public ShotModel
{
public:
    int getMeasurement()
    {
        return ShotModel::getMeasurement();
    }
};

class TestShotMeasurement : public TestAsync
{
public:
    TestShotMeasurement( TestShotModel& shot ) : mShot(shot)
    {
    }

    bool condition()
    {
        return mShot.getMeasurement() > 0;
    }

private:
    TestShotModel& mShot;
};

void TestMain::testMeasureShot()
{
    initGame(
      "[T>.........[\\S\n"
      " M .........[/S\n"
    );
    TestShotModel shot;
    shot.setParent( &mRegistry );
    TestShotMeasurement testShotMeasurement( shot );
    shot.fire( &mRegistry.getTank() );
    QVERIFY( testShotMeasurement.test() );
    cout << "measurement=" << shot.getMeasurement() << endl;
    QVERIFY( shot.getMeasurement() == 20 );
}

void TestMain::testCannonShot()
{
    initGame( "[T>.M..<\n" );
    MoveController& moveController = mRegistry.getMoveController();
    QSignalSpy killSpy( &mRegistry.getActiveCannon().getShot(), &ShotModel::tankKilled );
    QSignalSpy idleSpy( &moveController, &MoveController::idle );
    moveController.move(90);
    moveController.fire(2);
    moveController.move(-1);
    mRegistry.getSpeedController().setHighSpeed( true );
    idleSpy.wait(2000);
    if ( mRegistry.getTank().getCol() < 2 ) {
        idleSpy.wait(2000);
    }
    QVERIFY2( idleSpy.count() == 1, "move still in progress" );
    QVERIFY2( killSpy.empty(), "tank unexpectedly killed" );
    QVERIFY2( mRegistry.getTank().getCol() == 2, "tank didn't finish moves" );
}

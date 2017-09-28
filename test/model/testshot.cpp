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

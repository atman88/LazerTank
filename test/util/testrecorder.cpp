#include <iostream>
#include "../testmain.h"
#include "util/recorderprivate.h"

using namespace std;

class TestRecorderConsumer : public RecorderConsumer
{
public:
    TestRecorderConsumer() : mMoveCallCount(0), mFireCallCount(0), mReplayOnCallCount(0), mLastMoveDirection(0),
      mLastFireCount(0), mLastReplayOn(false)
    {
    }

    void move( int direction, bool doWakeup = true )
    {
        cout << "TestRecordConsumer: move(" << direction << "," << doWakeup << ")" << endl;
        ++mMoveCallCount;
        mLastMoveDirection = direction;
    }

    void fire( int count )
    {
        cout << "TestRecordConsumer: fire(" << count << ")" << endl;
        ++mFireCallCount;
        mLastFireCount = count;
    }

    void setReplay( bool on )
    {
        cout << "TestRecordConsumer: setReplay(" << on << ")" << endl;
        ++mReplayOnCallCount;
        mLastReplayOn = on;
    }

    int mMoveCallCount;
    int mFireCallCount;
    int mReplayOnCallCount;

    int mLastMoveDirection;
    int mLastFireCount;
    bool mLastReplayOn;
};

void TestMain::testRecorder()
{
    // test the bit field macros:
    EncodedMove move;
    move.clear();

    for( int i = 0; i < MAX_MOVE_SHOT_COUNT; ++i ) {
        ++move.u.move.shotCount;
    }
    // confirm capacity
    QVERIFY2( move.u.move.shotCount == MAX_MOVE_SHOT_COUNT, "MAX_MOVE_SHOT_COUNT too small" );
    // confirm it overflows here:
    QVERIFY2( ++move.u.move.shotCount == 0, "MAX_MOVE_SHOT_COUNT too big");

    move.clear();
    for( int i = 0; i < MAX_CONTINUATION_SHOT_COUNT; ++i ) {
        ++move.u.continuation.shotCount;
    }
    // confirm capacity
    QVERIFY2( move.u.continuation.shotCount == MAX_CONTINUATION_SHOT_COUNT, "MAX_MOVE_SHOT_COUNT too small" );
    // confirm it overflows here:
    QVERIFY2( ++move.u.continuation.shotCount == 0, "MAX_CONTINUATION_SHOT_COUNT too big");


    Recorder recorder( 2 );
    QVERIFY( recorder.isEmpty() );
    QVERIFY( recorder.getCount() == 0 );

    // fill to the overflow point:
    recorder.recordMove( true, 180 );
    recorder.recordMove( true, -1 );
    // cause a shot continuation (worst case):
    int shotCount;
    for( shotCount = 0; shotCount <= MAX_MOVE_SHOT_COUNT; ++shotCount ) {
        recorder.recordShot();
    }

    int count = recorder.getCount();
    // it should discover the overflow with this:
    recorder.recordMove( true, -1 );
    QVERIFY2( recorder.getCount() == count, "count overflowed" );

    RecorderReader* reader = recorder.getReader();
    TestRecorderConsumer consumer;
    QVERIFY( reader->readNext( &consumer ) );
    QVERIFY( consumer.mLastMoveDirection == 180 );

    QVERIFY( reader->readNext( &consumer ) );
    QVERIFY( consumer.mLastMoveDirection == 180 );
    QVERIFY( consumer.mLastFireCount == shotCount );

    QVERIFY2( reader->readNext( &consumer ) == false, "didn't hit expected end" );
}

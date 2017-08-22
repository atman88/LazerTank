#include <iostream>
#include "../testmain.h"
#include "util/recorderprivate.h"

using namespace std;

class TestRecorderPlayer : public RecorderPlayer
{
public:
    TestRecorderPlayer() : mMoveCallCount(0), mFireCallCount(0), mReplayOnCallCount(0), mLastMoveDirection(0),
      mLastFireCount(0), mLastReplayOn(false)
    {
    }

    void move( int direction, bool doWakeup = true )
    {
        cout << "TestRecorderPlayer: move(" << direction << "," << doWakeup << ")" << endl;
        ++mMoveCallCount;
        mLastMoveDirection = direction;
    }

    void fire( int count )
    {
        cout << "TestRecorderPlayer: fire(" << count << ")" << endl;
        ++mFireCallCount;
        mLastFireCount = count;
    }

    bool setReplay( bool on )
    {
        bool rc = mLastReplayOn;
        cout << "TestRecorderPlayer: setReplay(" << on << ")" << endl;
        ++mReplayOnCallCount;
        mLastReplayOn = on;
        return rc;
    }

    int mMoveCallCount;
    int mFireCallCount;
    int mReplayOnCallCount;

    int mLastMoveDirection;
    int mLastFireCount;
    bool mLastReplayOn;
};

/**
 * @brief Verify the bit field macros are consistent with their field definitions
 */
void TestMain::testRecorderBitFields()
{
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
}

/**
 * @brief Verify that the encoded record packs to a single byte as expected
 */
void TestMain::testRecorderRecordSize()
{
    QVERIFY( sizeof(EncodedMove) == 1 );
}

/**
 * @brief Test recorder overflow detection/handling
 */
void TestMain::testRecorderOverflow()
{
    RecorderPrivate& recorder_p = *new RecorderPrivate( 2 );
    QVERIFY( recorder_p.isEmpty() );
    QVERIFY( recorder_p.getCount() == 0 );

    Recorder& recorder = *new Recorder( &recorder_p );
    mRegistry.injectRecorder( &recorder );

    // fill to the overflow point:
    recorder_p.recordMove( true, 180 );
    recorder_p.recordMove( true, -1 );
    // cause a shot continuation (worst case):
    int shotCount;
    for( shotCount = 0; shotCount <= MAX_MOVE_SHOT_COUNT; ++shotCount ) {
        recorder_p.recordShot();
    }

    int count = recorder_p.getCount();
    // it should discover the overflow with this:
    recorder_p.recordMove( true, -1 );
    QVERIFY2( recorder_p.getCount() == count, "count overflowed" );

    RecorderReader* reader = new RecorderReader( 0, *recorder.source() );
    TestRecorderPlayer player;
    QVERIFY( reader->consumeNext( &player ) );
    QVERIFY( player.mLastMoveDirection == 180 );

    QVERIFY( reader->consumeNext( &player ) );
    QVERIFY( player.mLastMoveDirection == 180 );
    QVERIFY( player.mLastFireCount == shotCount );

    QVERIFY2( reader->consumeNext( &player ) == false, "didn't hit expected end" );

    delete reader;
}

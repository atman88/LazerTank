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

    void move( int direction ) override
    {
        cout << "TestRecorderPlayer: move(" << direction << ")" << endl;
        ++mMoveCallCount;
        mLastMoveDirection = direction;
    }

    void fire( int count ) override
    {
        cout << "TestRecorderPlayer: fire(" << count << ")" << endl;
        ++mFireCallCount;
        mLastFireCount = count;
    }

    bool setReplay( bool on ) override
    {
        bool rc = mLastReplayOn;
        cout << "TestRecorderPlayer: setReplay(" << on << ")" << endl;
        ++mReplayOnCallCount;
        mLastReplayOn = on;
        return rc;
    }

    bool readerFinished() override
    {
        return setReplay(false);
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
    QVERIFY2( ++move.u.continuation.shotCount == 0, "MAX_CONTINUATION_SHOT_COUNT didn't overflow");
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
    class MyRecorderPrivate : public RecorderPrivate
    {
    public:
        MyRecorderPrivate( int capacity ) : RecorderPrivate(capacity)
        {
        }

        int getCapacity() const
        {
            return mCapacity;
        }
    };

    MyRecorderPrivate& recorder_p = *new MyRecorderPrivate( 2 );
    QVERIFY( recorder_p.isEmpty() );
    QVERIFY( recorder_p.getAvailableCount() == 0 );

    Recorder& recorder = *new Recorder( &recorder_p );
    mRegistry.injectRecorder( &recorder );

    // fill to capacity:
    recorder_p.recordMove( true, 180 );
    for( int i = recorder_p.getCapacity(); --i >= 0; )
        recorder_p.recordMove( true, -1 );

    // cause a shot continuation (worst case):
    int shotCount;
    for( shotCount = 0; shotCount <= MAX_MOVE_SHOT_COUNT; ++shotCount ) {
        recorder_p.recordShot();
    }

    int count = recorder_p.getAvailableCount();
    // it should discover the overflow with this:
    recorder_p.recordMove( true, -1 );
    QVERIFY2( recorder_p.getAvailableCount() == count, "count overflowed" );

    auto reader = new RecorderReader( 0, *recorder.source() );
    TestRecorderPlayer player;
    QVERIFY( reader->consumeNext( &player ) );
    QVERIFY( player.mLastMoveDirection == 180 );

    QVERIFY( reader->consumeNext( &player ) );
    QVERIFY( player.mLastMoveDirection == 180 );
    QVERIFY( player.mLastFireCount == shotCount );

    QVERIFY2( reader->consumeNext( &player ) == false, "didn't hit expected end" );

    delete reader;
}

void TestMain::testRecorderAvailable()
{
    RecorderPrivate& recorder_p = *new RecorderPrivate( 16 );
    EncodedMove move;
    move.clear();
    move.u.move.adjacent = 1;

    const int preloadSize = 3;
    if ( char* p = recorder_p.getLoadableDestination( recorder_p.getLevel(), preloadSize ) ) {
        for( int i = 0; i < preloadSize; ++i )
            *p++ = static_cast<char>( move.u.value );
        recorder_p.releaseLoadableDestination( recorder_p.getLevel(), preloadSize );
        QCOMPARE( recorder_p.getAvailableCount(), preloadSize );

        // test non-destructive write
        recorder_p.recordMove( move.u.move.adjacent, -1 );
        QCOMPARE( recorder_p.getAvailableCount(), preloadSize );

        // test destructive write
        recorder_p.recordMove( move.u.move.adjacent, 90 );
        cout << recorder_p.getAvailableCount() << " available\n";
        QVERIFY( recorder_p.getAvailableCount() == 2 );
    } else {
        QFAIL( "getLoadableDestination returned null" );
    }
}

void TestMain::testRecorderContinuation()
{
    RecorderPrivate& recorder_p = *new RecorderPrivate( 16 );
    Recorder recorder( &recorder_p );

    for( int count = 0; count < MAX_MOVE_SHOT_COUNT+1; ++count )
        recorder_p.recordShot();
    QCOMPARE( recorder_p.getRecordedCount(), 2 );

    RecorderSource* source = recorder.source();
    EncodedMove move;
    move.u.value = source->get();
    QCOMPARE( move.u.move.shotCount, MAX_MOVE_SHOT_COUNT );

    EncodedMove continuation;   continuation.u.value = source->get();
    QVERIFY2( continuation.u.continuation.header == 0, "incorrect continuation header" );
    QCOMPARE( continuation.u.continuation.shotCount, 1 );
}

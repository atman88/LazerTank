#include <iostream>
#include "../testmain.h"
#include "util/recorderprivate.h"

using namespace std;

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
    recorder.recordMove( true, 180 );
    // cause a shot continuation (worst case):
    for( int i = 0; i <= MAX_MOVE_SHOT_COUNT; ++i ) {
        recorder.recordShot();
    }
    recorder.recordMove( true, 180 );
    // cause a shot continuation (worst case):
    for( int i = 0; i <= MAX_MOVE_SHOT_COUNT; ++i ) {
        recorder.recordShot();
    }

    int count = recorder.getCount();
    // it should discover the overflow with this:
    recorder.recordMove( true, 180 );
    QVERIFY2( recorder.getCount() == count, "count overflowed" );
}

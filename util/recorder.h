#ifndef RECORDER_H
#define RECORDER_H

#include "piece.h"

typedef struct EncodedMove {
    EncodedMove() : encodedAngle(0), shotCount(0)
    {
    }

    unsigned char encodedAngle:3;
    unsigned char shotCount:5;
} EncodedMove;

class Recorder;
class MoveController;

class RecorderReader
{
public:
    RecorderReader( Recorder* source );
    void rewind();
    bool readNext( MoveController* controller );

private:
    int mOffset;
    int mRecordedCount;
    EncodedMove mLastMove;
    Recorder* mSource;
};

class Recorder
{
public:
    Recorder() : mRecordedCount(0), mReader(0)
    {
    }
    ~Recorder();

    void reset();
    void addMove( int direction );
    void addShot();
    bool isEmpty() const;
    int getCount() const;

    RecorderReader* getReader();
    void closeReader();

private:
    EncodedMove mCurMove;
    int mRecordedCount;
    EncodedMove mRecorded[1024];
    RecorderReader* mReader;

    friend class RecorderReader;
};

#endif // RECORDER_H

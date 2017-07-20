#ifndef RECORDERPRIVATE_H
#define RECORDERPRIVATE_H

#include "recorder.h"

#define MAX_BITFIELD_VALUE(nbits) ((1<<nbits)-1)

//
// The purpose of the recorder is to save tank actions and to provide a reader for supporting autoreplay.
//
// A primary goal of the Recorder is to minimize memory requirements, so by design this recorder
// encodes the actions into a compressed format where each move begins with a move record which can be optionally
// followed by a continuation record if/when needed:
//
typedef struct EncodedMove {
    void clear()
    {
        u.value = 0;
    }

    bool isEmpty() const
    {
        return u.value == 0;
    }

    union {
        struct {
            // either of these first two bits being set identifies this move record
            unsigned char adjacent    :1; // 1 indicates moving to an new square identified by the prior recorded angle
            unsigned char rotate      :1; // 1 indicates rotating to the encoded angle

            unsigned char encodedAngle:2; // A value between 0-3 which corresponds to an angle of 0/90/180/270 respectively
            unsigned char shotCount   :4; // A number of times to fire at this move point
        } move;
// this macro needs to be defined using the above shotCount field's bit count:
#define MAX_MOVE_SHOT_COUNT MAX_BITFIELD_VALUE(4)

        struct {
            unsigned char header   :2; // 0 identifies this continuation record
            unsigned char shotCount:6; // Additional shot count to accumulate with its preceeding move record
        } continuation;
// this macro needs to be defined using the above shotCount field's bit count:
#define MAX_CONTINUATION_SHOT_COUNT MAX_BITFIELD_VALUE(6)

        unsigned char value;
    } u;
} EncodedMove;

class RecorderPrivate
{
public:
    /**
     * @brief Constructor
     * @param capacity Specify how much recording to allow. This value loosely translates into the number of move
     * sequences that can be recorded. It defaults to a reasonably generous number needed to complete any one of the
     * available packaged levels.
     */
    RecorderPrivate( int capacity );
    ~RecorderPrivate();

    /**
     * @brief Reset as appropriate on a board change
     */
    void onBoardLoaded( int initialDirection );

    /**
     * @brief Query whether anything has been recorded yet
     * @return true if has anything recorded
     */
    bool isEmpty() const;

    /**
     * @brief Get the number of recorded records for possible statistical purposes
     */
    int getCount() const;

    /**
     * @brief Obtain a reader for playback of this recording
     */
    RecorderReader* getReader();

    /**
     * @brief Release the reader resource
     */
    void closeReader();

    /**
     * @brief record a change in direction
     * @param adjacent Indicates if moving into the adjacent square identified by the last recorded tank position
     * @param rotation A value of 0, 90, 180 or 270 indicates a rotation to that angle. -1 indicates no rotation
     */
    void recordMove( bool adjacent, int rotation );

    /**
     * @brief record a single shot
     */
    void recordShot();

private:
    /**
     * @brief store the current move in the mRecorded records without committing the write offset
     * @return the offset past the written move. mRecordedCount is returned if unsuccessful.
     */
    int storeCurMove();

    /**
     * @brief Saves the lazily-written mCurMove/mCurContinuation pair to the record buffer
     * @return true if sucessful
     */
    bool commitCurMove();

    int mStartDirection;              // the initial tank direction
    EncodedMove mCurMove;             // the primary record for the current move being assembled
    EncodedMove mCurContinuation;     // companion to mCurMove, uses lazy initialization- in use if mCurMove's shotCount at max

    int mRecordedCount;               // number of records currently saved in the buffer
    EncodedMove* mRecorded;           // the recording buffer
    int mCapacity;                    // Configured recording limit
    int mRecordedAllocationWaterMark; // tracks allocations
    RecorderReader* mReader;          // the single active reader or 0 if not currently reading

    friend class Recorder;
    friend class RecorderReader;
};

#endif // RECORDERPRIVATE_H

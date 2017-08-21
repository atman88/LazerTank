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
     * @brief Associates subsequent recording with the given level number
     * If current buffered data is associated with a different level then buffered data is cleared otherwise a
     * non-destructive rewind is performed.
     */
    void onBoardLoaded( int level );

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
     * @brief Obtain a raw data reader for this recording
     */
    RecorderSource* source();

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

    /**
     * @brief Preloads the recorder with recording data
     * @param count The size of the data
     * @param data raw recording data
     * @return true if sucessful
     */
    bool setData( int count, const unsigned char *data );

    /**
     * @brief Query which level is being recorded
     * @return The level number
     */
    int getLevel() const;

private:
    /**
     * @brief Save the move at the given position with detection of overwrite of any prerecorded data
     * @param move The value to save
     * @param pos The offset to save
     * @return The updated offset
     */
    int storeInternal( EncodedMove move, int pos );

    /**
     * @brief Save the current move in the mRecorded records without committing the write offset
     * @return the offset past the written move. mWritePos is returned if unsuccessful.
     */
    int storeCurMove();

    /**
     * @brief Saves the lazily-written mCurMove/mCurContinuation pair to the record buffer
     * @return true if sucessful
     */
    bool commitCurMove();

    int mLevel;                       // the game level being recorded
    EncodedMove mCurMove;             // the primary record for the current move being assembled
    EncodedMove mCurContinuation;     // companion to mCurMove, uses lazy initialization- in use if mCurMove's shotCount at max

    int mWritePos;                    // offset into mRecorded where moves are being saved
    int mPreRecordedCount;            // if nonzero, holds the number moves preserved (in mRecorded). (Moves are preserved during playback.)
    EncodedMove* mRecorded;           // the recording buffer
    int mCapacity;                    // Configured recording limit
    int mRecordedAllocationWaterMark; // tracks allocations

    friend class Recorder;
    friend class RecorderSource;
};

#endif // RECORDERPRIVATE_H

#ifndef RECORDERPRIVATE_H
#define RECORDERPRIVATE_H

class PersistLevelLoader;

#include "recorder.h"
#include "loadable.h"
#include "encodedmove.h"
#include "gameregistry.h"

class RecorderPrivate : public Loadable
{
public:
    /**
     * @brief Constructor
     * @param capacity Specify how much recording to allow. This value loosely translates into the number of move
     * sequences that can be recorded. It defaults to a reasonably generous number needed to complete any one of the
     * available packaged levels.
     */
    RecorderPrivate( int capacity );
    ~RecorderPrivate() override;


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
    int getAvailableCount() const;

    /**
     * @brief Get the number of records
     * @return
     */
    int getRecordedCount() const;

    /**
     * @brief Flush any outstanding lazy write
     */
    void lazyFlush();

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

    bool ensureCapacity( int count );

    /**
     * @brief Preloads the recorder with recording data
     * @param loader The data source
     * @return true if sucessful
     */
    bool setData( PersistLevelLoader& loader );

    /**
     * @brief Query which level is being recorded
     * @return The level number
     */
    int getLevel() const;

    /**
     * @brief dump contents for debug
     */
    void dump();

    // Loadable interface
    char*getLoadableDestination(int forLevel, int count) override;
    void releaseLoadableDestination(int forLevel, int actualCount) override;

    int getPreRecordedCount() const;

    void backdoor( int code );

protected:
    int mCapacity;                    // Configured recording limit

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
    int mRecordedAllocationWaterMark; // tracks allocations

    friend class Recorder;
    friend class RecorderActiveSource;
    friend class RecorderPersistedSource;
};

class RecorderActiveSource : public RecorderSource
{
public:
    RecorderActiveSource( RecorderPrivate& recorder );
    int getCount() const override;
    unsigned char get() override;
    ReadState getReadState() const override;

protected:
    void doDataReady() override;
};

class RecorderPersistedSource : public RecorderSource
{
public:
    RecorderPersistedSource( RecorderPrivate& recorder, Persist& persist );
    ~RecorderPersistedSource() override;

    int getCount() const override;
    unsigned char get() override;
    ReadState getReadState() const override;

protected:
    void doDataReady() override;

    Persist& mPersist;
    PersistLevelLoader* mLoader;
    int mLoadSequence;
};

#endif // RECORDERPRIVATE_H

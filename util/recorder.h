#ifndef RECORDER_H
#define RECORDER_H

class Recorder;
class MoveController;
struct EncodedMove;
class RecorderPrivate;

class RecorderReader
{
public:
    RecorderReader( RecorderPrivate* source );
    /**
     * @brief Reposition to the start of the recording
     */
    void rewind();

    /**
     * @brief Perform the next recorded move sequence
     * @param controller The move controller used to perform the move
     * @return true if the next sequence was performed or false if at the end of the recording or an error was encountered
     */
    bool readNext( MoveController* controller );

private:
    /**
     * @brief error state handler
     */
    void abort();

    /**
     * @brief Read the next encoded move record
     * @return The next record
     */
    EncodedMove readInternal();

    int mOffset;
    int mRecordedCount;
    int mLastDirection;
    RecorderPrivate* mSource;
};

class Recorder
{
public:
    /**
     * @brief Constructor
     * @param capacity Specify how much recording to allow. This value loosely translates into the number of move
     * sequences that can be recorded. It defaults to a reasonably generous number needed to complete any one of the
     * available packaged levels.
     */
    Recorder( int capacity = 1024 );
    ~Recorder();

    /**
     * @brief Restore this reader to its empty state
     */
    void reset();

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
     * @brief Query whether this recorder has recorded anything yet
     * @return true if has recorded anything otherwise false
     */
    bool isEmpty() const;

    /**
     * @brief Get the number of recorded records for possible statistical purposes
     */
    int getCount() const;

    /**
     * @brief Get the total number of records this recorder can hold
     */
    int getCapacity() const;

    /**
     * @brief Obtain a reader for playback of this recording
     */
    RecorderReader* getReader();

    /**
     * @brief Release the reader resource
     */
    void closeReader();

private:
    RecorderPrivate* mPrivate;
};

#endif // RECORDER_H

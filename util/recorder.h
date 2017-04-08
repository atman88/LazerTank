#ifndef RECORDER_H
#define RECORDER_H

class Recorder;
struct EncodedMove;
class RecorderPrivate;

/**
 * @brief The RecorderReader interface
 */
class RecorderConsumer
{
public:
    /**
     * @brief Serves a move to the consumer
     * @param direction A rotation angle (one of 0, 90, 180, 270) or -1 to advance in the current direction
     */
    virtual void move( int direction, bool doWakeup = true ) = 0;

    /**
     * @brief Updates the consumer on the replay state
     * @param on
     */
    virtual void setReplay( bool on ) = 0;

    /**
     * @brief Serves a shot to the consumer
     * @param count The number of times to shoot
     */
    virtual void fire( int count ) = 0;
};

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
     * @param consumer The object to receive the move
     * @return true if the next sequence was performed or false if at the end of the recording or an error was encountered
     */
    bool readNext( RecorderConsumer* consumer );

    /**
     * @brief Obtain the read position
     * @return
     */
    int getOffset() const;

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
     * sequences that can be recorded. It should default to a generous number needed to complete any one of the
     * available levels.
     */
    Recorder( int capacity = 8000 );
    ~Recorder();

    /**
     * @brief Reset as appropriate on a board change
     */
    void onBoardLoaded();

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

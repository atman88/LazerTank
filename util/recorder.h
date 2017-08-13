#ifndef RECORDER_H
#define RECORDER_H

#include <QtPlugin>

class Recorder;
class RecorderPrivate;
class Board;

/**
 * @brief The RecorderReader interface
 */
class RecorderPlayer
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

class RecorderSource
{
public:
    typedef enum {
        Ready,
        Pending,
        Finished
    } ReadState;

    virtual ~RecorderSource(){}

    /**
     * @brief Query whether data is ready to read
     */
    virtual ReadState getReadState() = 0;

    /**
     * @brief Query the total count of moves that can be read
     */
    virtual int getCount() = 0;

    /**
     * @brief Get the current read position
     */
    virtual int pos() = 0;

    /**
     * @brief Read the next available data value
     * @return The next move or an empty move is returned if no more moves ready to read.
     */
    virtual unsigned char get() = 0;

    /**
     * @brief Undo the last get
     */
    virtual void unget() = 0;

    /**
     * @brief Reposition to the beginning of the data
     */
    virtual void rewind() = 0;

    /**
     * @brief Reposition to the end of the data
     * @return The end position
     */
    virtual int seekEnd() = 0;

signals:
    /**
     * @brief Notifies that data is ready to read
     */
    virtual void dataReady() = 0;
};
Q_DECLARE_INTERFACE(RecorderSource,"RecorderSource")

class RecorderReader
{
public:
    RecorderReader( int startDirection, RecorderSource& source );
    ~RecorderReader();

    /**
     * @brief Reposition to the start of the recording
     */
    void rewind();

    /**
     * @brief Reads the next recorded move sequence and drives the player
     * @param player The object to receive the move
     * @return true if the next sequence was performed or false if at the end of the recording or an error was encountered
     */
    bool consumeNext( RecorderPlayer* player );

    /**
     * @brief Obtain the read position
     */
    int pos() const;

private:
    /**
     * @brief error state handler
     */
    void abort();

    int mLastDirection;
    RecorderSource& mSource;
};

class Recorder
{
public:
    static const int SaneMaxCapacity = 8000;

    /**
     * @brief Constructor
     * @param capacity Specify how much recording to allow. This value loosely translates into the number of move
     * sequences that can be recorded. It should default to a generous number needed to complete any one of the
     * available levels.
     */
    Recorder( int capacity = SaneMaxCapacity );
    ~Recorder();

    /**
     * @brief React as appropriate on a board change
     */
    void onBoardLoaded( Board& board );

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
     * @brief Query which level is being recorded
     * @return The level number
     */
    int getLevel() const;

    /**
     * @brief Get the total number of records this recorder can hold
     */
    int getCapacity() const;

    /**
     * @brief Obtain a raw data reader for this recording
     */
    RecorderSource* source();

    /**
     * @brief Obtain a reader for playback of this recording
     */
    RecorderReader* getReader();

    /**
     * @brief copy the raw recording data
     * @param buf The destination to copy the data to or 0 to query the required size without copying.
     * @return The size of the data
     */
    int getData( unsigned char *data );

    /**
     * @brief Preloads the recorder with recording data
     * @param count The size of the data
     * @param data raw recording data
     * @return true if sucessful
     */
    bool setData( int count, const unsigned char *data );

private:
    RecorderPrivate* mPrivate;
};

#endif // RECORDER_H

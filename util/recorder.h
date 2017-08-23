#ifndef RECORDER_H
#define RECORDER_H

#include <QObject>

class Recorder;
class Persist;
class PersistLevelLoader;
class RecorderPrivate;
class Board;

/**
 * @brief The RecorderPlayer interface
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
     * @brief Updates the consumer's replay state
     * @param on Replay is enabled if true otherwise replay is disabled
     * @return The previous replay on/off state
     */
    virtual bool setReplay( bool on ) = 0;

    /**
     * @brief Serves a shot to the consumer
     * @param count The number of times to shoot
     */
    virtual void fire( int count ) = 0;
};

class RecorderSource : public QObject
{
    Q_OBJECT

public:
    typedef enum {
        Ready,
        Pending,
        Finished
    } ReadState;

    RecorderSource( RecorderPrivate& recorder, Persist& persist );
    ~RecorderSource();

    /**
     * @brief Query whether data is ready to read
     */
    ReadState getReadState() const;

    /**
     * @brief Query the total count of buffered moves
     */
    int getCount();

    /**
     * @brief Get the current read position
     */
    int pos();

    /**
     * @brief Read the next available data value
     * @return The next move or an empty move is returned if no more moves ready to read.
     */
    unsigned char get();

    /**
     * @brief Undo the last get
     */
    void unget();

    /**
     * @brief Reposition to the beginning of the data
     */
    void rewind();

    /**
     * @brief Reposition to the end of the data
     * @return The end position
     */
    int seekEnd();

signals:
    /**
     * @brief Notifies that the data is now ready to read
     */
    void dataReady();

private slots:
    void onDataReady();

private:
    RecorderPrivate& mRecorder;
    Persist& mPersist;
    PersistLevelLoader* mLoader;
    int mOffset;
    int mLoadSequence;
};

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

class Recorder : public QObject
{
    Q_OBJECT

public:
    static const int SaneMaxCapacity = 8000;

    /**
     * @brief Constructor
     * @param capacity Specify how much recording to allow. This value loosely translates into the number of move
     * sequences that can be recorded. It should default to a generous number needed to complete any one of the
     * available levels.
     */
    Recorder( int capacity = SaneMaxCapacity );
    Recorder( RecorderPrivate* recorder_p );
    ~Recorder();

    /**
     * @brief React as appropriate on a board change
     */
    void onBoardLoaded( int level );

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
     * @return A newly allocated RecorderSource instance if successful or 0 if an failure occured.
     * Onus is on the caller to delete the returned RecorderSource
     */
    RecorderSource* source();

protected:
    RecorderPrivate* mPrivate;
    int mStartDirection; // the initial tank direction
};

#endif // RECORDER_H

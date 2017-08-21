#ifndef PERSIST_H
#define PERSIST_H

#include <memory>
#include <QObject>
#include <QString>
#include <QTime>

class Runnable;
class Persist;
class PersistentUpdateRunnable;
class GameRegistry;
class RecorderSource;
class LoadLevelRunnable;

#include "recorder.h"

typedef struct PersistedLevelIndexFooter {
    static const unsigned char MajorVersionValue = 1;
    static const unsigned char MinorVersionValue = 0;

    int16_t       count;
    unsigned char majorVersion:4;
    unsigned char minorVersion:4;
    char          magic[3];
} PersistedLevelIndexFooter;

typedef struct {
    std::int16_t level;
    std::int16_t size;
    std::int32_t offset;
} PersistedLevelIndex;

typedef struct PersistedLevelRecord {
    static const unsigned char MajorVersionValue = 1;
    static const unsigned char MinorVersionValue = 0;

    char          magic[3];
    unsigned char majorVersion:4;
    unsigned char minorVersion:4;
    int16_t       level;
    int16_t       count;
    unsigned char moves[0];
} PersistedLevelRecord;

class PersistLevelLoader : public QObject
{
    Q_OBJECT

public:
    PersistLevelLoader( Persist& persist, PersistedLevelIndex index, QObject* parent = 0 );
    virtual ~PersistLevelLoader()
    {
    }

    int getCount();
    bool load( char *buf, int count );

signals:
    void dataReadyInternal();
    void dataReady();

private slots:
    void onDataReadyInternal();

private:
    void onLoaded();

    Persist& mPersist;
    PersistedLevelIndex mIndex;
    LoadLevelRunnable* mRunnable;

    friend class LoadLevelRunnable;
};

class Persist : public QObject
{
    Q_OBJECT

public:
    Persist( const char* path = 0, QObject* parent = 0 );
    ~Persist();
    void init( GameRegistry* registry );

    QString getPath() const;

    QTime lastUpdateTime() const;

    PersistLevelLoader* getLevelLoader( int level );

    bool isUnusable() const;

    bool updateInProgress() const;

public slots:
    void onLevelUpdated( int number );
    void setUnusable();

signals:
    /**
     * @brief Notify that the level is known to have completed
     */
    void levelSetComplete( int level );

    /**
     * @brief Internal signal from background to foreground when the indices have been read or changed
     */
    void indexReadyQueued();

private slots:
    /**
     * @brief Recieves new indexes from its current runnable
     */
    void onIndexReadyQueued();

private:
    /**
     * @brief Managed running of a runnable
     * @param runnable The runnable to run
     * @param registry The registry if known (optimizes case where already known)
     */
    void doUpdate( PersistentUpdateRunnable* runnable, GameRegistry* registry = 0 );

    /**
     * @brief Called by the background to notify that an update is completed
     * @param level
     */
    void onPersistentUpdateResult();

    static const int UnusableFileSize = -2;

    QString mPath;
    bool mFileUnusable;
    std::map<int,PersistedLevelIndex> mRecords;
    PersistedLevelIndexFooter mFooter;
    QTime mLastUpdateTime;
    PersistentUpdateRunnable* mUpdateRunnable;
    std::shared_ptr<Runnable> mSharedRunnable;

    friend class PersistentRunnable;
    friend class PersistentUpdateRunnable;
    friend class InitRunnable;
    friend class UpdateRunnable;
};

#endif // PERSIST_H

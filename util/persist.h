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
class LoadLevelRunnable;

#include "loadable.h"
#include "persistfile.h"

class PersistLevelLoader : public QObject
{
    Q_OBJECT

public:
    PersistLevelLoader( Persist& persist, PersistedLevelIndex index, QObject* parent = nullptr );

    /**
     * @brief Query the size of the persisted recording data for the level associated with this loader
     * @return The persisted recording size or 0 if not persisted
     */
    int getCount();

    /**
     * @brief Initiate loading for this loader's associated level
     * @param loadable Receives the recording data being loaded
     * @return true if successful
     */
    bool load( Loadable& loadable );

signals:
    /**
     * @brief Notifies that the load has completed
     */
    void dataReady();

    /**
     * @brief Internal across-thread signal
     */
    void dataReadyInternal();

private slots:
    void onDataReadyInternal();

private:
    void onLoaded();

    Persist& mPersist;
    PersistedLevelIndex mIndex;
    bool mStarted;

    friend class LoadLevelRunnable;
};

class Persist : public QObject
{
    Q_OBJECT

public:
    Persist( const char* path = nullptr, QObject* parent = nullptr );
    ~Persist() override = default;
    void init( GameRegistry* registry );

    /**
     * @brief Get the path of the persisted file
     * @return
     */
    QString getPath() const;

    /**
     * @brief Obtain the time that the persisted file was last written
     */
    QTime lastUpdateTime() const;

    /**
     * @brief Query whether the given level is persisted
     * @param level The number identifying a desired level
     * @return true if the level is persisted
     */
    bool isPersisted( int level ) const;

    /**
     * @brief Obtain a loader for reading the given level from persistent storage
     * @param level The number of the level to load
     * @return A newly allocated PersistLevelLoader instance if successful or 0 if an failure occured.
     * Onus is on the caller to delete the returned PersistLevelLoader
     */
    PersistLevelLoader* getLevelLoader( int level );

    /**
     * @brief Query whether the persisted file path has been found to be inoperable
     * @return true if we've given up on attempting to maintain the file
     */
    bool isUnusable() const;

    /**
     * @brief Query whether this loader is busy
     * @return true if loading has been initiated and is yet to complete, otherwise false
     */
    bool updateInProgress() const;

public slots:
    /**
     * @brief Trigger a save for the given level number
     * @param number The level to save
     */
    void onLevelUpdated( int number );

    /**
     * @brief Mark the persisted file as being inoperable. Further access to the file will be inhibited as a result of this call
     */
    void setUnusable();

signals:
    /**
     * @brief Notifies that the level is known to have completed
     * @param level The level number
     * @param moveCount The number of moves taken to complete the level
     */
    void levelSetComplete( int level, int count );

    /**
     * @brief Internal signal from background to foreground fired when the indices have been read or changed
     */
    void indexReadyQueued();

private slots:
    /**
     * @brief Receives new indexes from its current runnable
     */
    void onIndexReadyQueued();

private:
    /**
     * @brief Managed running of a runnable
     * @param runnable The runnable to run
     * @param registry The registry if known (optimizes case where already known)
     */
    void doUpdate( PersistentUpdateRunnable* runnable, GameRegistry* registry = nullptr );

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

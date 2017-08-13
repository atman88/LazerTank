#ifndef PERSIST_H
#define PERSIST_H

#include <memory>
#include <QObject>
#include <QString>
#include <QTime>

class GameRegistry;
class InitRunnable;

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

class Persist : public QObject
{
    Q_OBJECT

public:
    Persist( const char* path = 0, QObject* parent = 0 );
    ~Persist();
    void init( GameRegistry* registry );

    int getFileSize() const;

    QString getPath() const;

    QTime lastUpdateTime() const;

public slots:
    void onLevelUpdated( int number );

signals:
    /**
     * @brief Notify that the level is known to have completed
     */
    void levelSetComplete( int level );

private:
    void onIndexRead( PersistedLevelIndex index );

    QString mPath;
    int mFileSize;
    std::map<int,PersistedLevelIndex> mRecords;
    PersistedLevelIndexFooter mFooter;
    QTime mLastUpdateTime;

    friend class PersistentRunnable;
    friend class InitRunnable;
    friend class UpdateRunnable;
};

#endif // PERSIST_H

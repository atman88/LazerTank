#include <iostream>
#include <cstring>
#include <QFile>
#include <QFile>
#include <QDir>

#include "persist.h"
#include "workerthread.h"
#include "recorder.h"
#include "controller/gameregistry.h"
#include "controller/movecontroller.h"

static const char PersistentIndexFooterMagicValue[3] = { 'L', 'T', 'i' };
static const char PersistentLevelRecordMagicValue[3] = { 'L', 'T', 'r' };

// A specialized PersistedLevelIndex that sorts by offset:
typedef struct RunnableIndex {
    PersistedLevelIndex rec;

    // offset compare
    constexpr bool operator<( const struct RunnableIndex& other ) const
    {
        return rec.offset < other.rec.offset;
    }
} RunnableIndex;

class PersistentRunnable : public ErrorableRunnable
{
public:
    enum {
        OpenCode = 1,
        SeekFooterCode,
        ReadFooterCode,
        FooterMagicCode,
        FooterMajorCode,
        SeekIndexCode,
        ReadIndexCode,
        SeekRecordCode,
        ReadRecordCode,
        RecordMagicCode,
        RecordMajorCode,
        RecordLevelCode,
        RecordSizeCode,
        ReadLevelCode,
        NullSourceCode,
        ErrorCodeUpperBound // must be last
    } ErrorCode;

    const int SilenceErrorFlag = 0x1000;

    static const int SaneMaxFileSize = 100 *
      (sizeof(PersistedLevelRecord)+Recorder::SaneMaxCapacity + sizeof(PersistedLevelIndex)) + sizeof(PersistedLevelIndexFooter);

    PersistentRunnable( Persist& persist ) : mPersist(persist), mFile( persist.mPath )
    {
    }
    virtual ~PersistentRunnable() {}

    void runInternal() override
    {
        ErrorableRunnable::runInternal();
        mFile.close(); // ensure closed without delay
    }

    void warn( QString msg )
    {
        std::cout << qPrintable(mFile.fileName()) << ": " << qPrintable(msg);
        if ( mFile.error() != QFileDevice::NoError ) {
            std::cout << ": " << qPrintable(mFile.errorString()) << " (pos=" << mFile.pos() << ")";
        }
        std::cout << std::endl;
    }

    virtual void onError( int errorCode ) override
    {
        QString msg;
        switch( errorCode ) {
        case OpenCode:        msg = "open"; mPersist.setUnusable();                                           break;
        case SeekFooterCode:  msg = "seek to footer";                                                         break;
        case FooterMagicCode: msg = "not a saved game";                                                       break;
        case ReadFooterCode:  msg = "couldn't read footer";                                                   break;
        case FooterMajorCode: msg = QString("incompatible found=%1").arg(mPersist.mFooter.majorVersion);      break;
        case SeekIndexCode:   msg = QString("couldn't seek to indexes count=%1").arg(mPersist.mFooter.count); break;
        case ReadIndexCode:   msg = "couldn't read index";                                                    break;
        case SeekRecordCode:  msg = "seek to record failed";                                                  break;
        case ReadRecordCode:  msg = "couldn't read record";                                                   break;
        case RecordMagicCode: msg = "invalid record";                                                         break;
        case RecordMajorCode: msg = "unsupported record version";                                             break;
        case RecordLevelCode: msg = "record level mismatch";                                                  break;
        case RecordSizeCode:  msg = "record size mismatch";                                                   break;
        case ReadLevelCode:   msg = "could't read level";                                                     break;
        case NullSourceCode:  msg = "null source";                                                            break;
        default:
            if ( errorCode & SilenceErrorFlag ) {
                return;
            }
            msg = QString("unspecified error %1").arg(errorCode);
        }
        warn( msg );
    }

    void eOpen( QIODevice::OpenModeFlag mode )
    {
        if ( !mFile.open( mode ) ) {
            error( OpenCode );
        }
    }

    void eSeek( int offset, int errorCode )
    {
        if ( !mFile.seek( offset ) ) {
            error( errorCode );
        }
    }

    void eRead( char* data, int size, int errorCode )
    {
        if ( mFile.read( data, size ) != size ) {
            error( errorCode );
        }
    }

    Persist& mPersist;
    QFile mFile;
};

class PersistentUpdateRunnable : public PersistentRunnable
{
public:
    PersistentUpdateRunnable( Persist& persist ) : PersistentRunnable(persist)
    {
    }

    std::set<RunnableIndex> mIndexes;

    void runInternal() override
    {
        PersistentRunnable::runInternal();
        mPersist.onPersistentUpdateResult();
    }
};

class InitRunnable : public PersistentUpdateRunnable
{
public:
    InitRunnable( Persist& persist ) : PersistentUpdateRunnable(persist)
    {
    }

    void run() override
    {
        if ( mFile.exists() ) {
            eOpen( QIODevice::ReadOnly );

            auto fileSize = mFile.size();
            if ( fileSize > SaneMaxFileSize ) {
                warn( QString("insane size %1").arg( fileSize ) );
                mPersist.mFileUnusable = true;
            } else if ( ((unsigned) fileSize) < sizeof(PersistedLevelIndexFooter) ) {
                warn( QString("size %1 too small to read").arg( fileSize ) );
            } else {
                int fileSize = (int) mFile.size();
                eSeek( fileSize-sizeof(PersistedLevelIndexFooter), SeekFooterCode );
                eRead( (char*) &mPersist.mFooter, sizeof mPersist.mFooter, ReadFooterCode );
                if ( std::memcmp( mPersist.mFooter.magic, PersistentIndexFooterMagicValue, sizeof mPersist.mFooter.magic ) ) {
                    error( FooterMagicCode );
                }
                if ( mPersist.mFooter.majorVersion != PersistedLevelIndexFooter::MajorVersionValue ) {
                    error( FooterMajorCode );
                }

                eSeek( fileSize - (sizeof mPersist.mFooter) - mPersist.mFooter.count * sizeof(PersistedLevelIndex), SeekIndexCode );
                for( int i = 0; i < mPersist.mFooter.count; ++i ) {
                    RunnableIndex index;
                    eRead( (char*) &index, sizeof index, ReadIndexCode );
                    mIndexes.insert( index );
                }
            }
        }
    }
};

class UpdateRunnable : public PersistentUpdateRunnable
{
public:
    UpdateRunnable( int level, Persist& persist, Recorder& sourceRecorder )
      : PersistentUpdateRunnable(persist), mLevel(level), mSourceRecorder(sourceRecorder), mSource(0)
    {
    }

    ~UpdateRunnable()
    {
        if ( mSource ) {
            delete mSource;
        }
    }

    void run() override
    {
        // build a list of retained indexes sorted by offset
        PersistedLevelIndex oldIndex;
        oldIndex.offset = -1; // mark as null
        for( auto it : mPersist.mRecords ) {
            if ( it.first == mLevel ) {
                oldIndex = it.second;
            } else {
                // optimize?
                RunnableIndex newIndex;
                newIndex.rec = it.second;
                mIndexes.insert( newIndex );
            }
        }

        RunnableIndex newIndex;
        newIndex.rec.level = mLevel;
        newIndex.rec.offset = -1; // mark not set
        newIndex.rec.size = sizeof(PersistedLevelRecord) + mSourceRecorder.getAvailableCount();

        mSource = mSourceRecorder.source();
        if ( !mSource ) {
            error( NullSourceCode );
        }

        bool simplyOverwrite = false;
        if ( !mFile.exists() ) {
            eOpen( QIODevice::WriteOnly );
            newIndex.rec.offset = 0;
            writeLevel( newIndex );
        } else {
            eOpen( QIODevice::ReadWrite );
            int writeOffset = oldIndex.offset;
            if ( writeOffset >= 0 ) {
                simplyOverwrite = oldIndex.size >= newIndex.rec.size;
                if ( simplyOverwrite ) {
                    newIndex.rec.offset = writeOffset;
                } else {
                    for( auto it : mIndexes ) {
                        if ( it.rec.offset > oldIndex.offset ) {
                            moveRecord( it.rec, writeOffset );
                            it.rec.offset = writeOffset;
                            writeOffset += it.rec.size;
                        }
                    }
                }
            }
            if ( newIndex.rec.offset < 0 ) {
                if ( !mIndexes.size() ) {
                    newIndex.rec.offset = 0;
                } else {
                    auto it = mIndexes.end();
                    --it;
                    newIndex.rec.offset = it->rec.offset + it->rec.size;
                }
            }
            writeLevel( newIndex );

            if ( !simplyOverwrite ) {
                writeOffset = mFile.pos();
            } else {
                writeOffset = newIndex.rec.offset + newIndex.rec.size;
            }
            // ensure the index list lands at the very end of the file
            int listSize = mIndexes.size() * sizeof(PersistedLevelIndex) + (sizeof mPersist.mFooter);
            if ( (mFile.size() - writeOffset) > listSize ) {
                writeOffset = (int) (mFile.size() - listSize);
            }

            eSeek( writeOffset, SeekIndexCode );
        }

        // write the index
        for( auto it : mIndexes ) {
            mFile.write( (const char*) &it.rec, sizeof it.rec );
        }

        // write the footer
        std::memcpy( &mPersist.mFooter.magic, PersistentIndexFooterMagicValue, sizeof mPersist.mFooter.magic );
        mPersist.mFooter.majorVersion = PersistedLevelIndexFooter::MajorVersionValue;
        mPersist.mFooter.minorVersion = PersistedLevelIndexFooter::MinorVersionValue;
        mPersist.mFooter.count = mIndexes.size();
        mFile.write( (const char*) &mPersist.mFooter, sizeof mPersist.mFooter );
    }

    void onError( int errorCode ) override
    {
        switch( errorCode ) {
        case SourceLevelCode:
            warn( QString("premature end of source count=%1 filePos=%2").arg(mSource->getCount()).arg(mFile.pos()) );
            errorCode |= SilenceErrorFlag;
            break;
        case UpdateLevelSanity:
            warn( QString("underlying recorder level changed %1->%2").arg(mLevel).arg(mSourceRecorder.getLevel()) );
            errorCode |= SilenceErrorFlag;
            break;
        default:
            break;
        }
        PersistentUpdateRunnable::onError( errorCode );
    }

private:
    void moveRecord( PersistedLevelIndex& index, int newOffset )
    {
        char buf[256];

        int rpos = index.offset;
        int wpos = newOffset;
        int remaining = index.size;
        int chunkSize = sizeof buf;
        while( remaining > 0 ) {
            if ( chunkSize > remaining ) {
                chunkSize = remaining;
            }
            eSeek( rpos, SeekRecordCode );
            eRead( buf, chunkSize, ReadRecordCode );
            rpos += chunkSize;
            eSeek( wpos, SeekRecordCode );
            mFile.write( buf, chunkSize );
            wpos += chunkSize;

            remaining -= chunkSize;
        }
    }

    void writeLevel( RunnableIndex& newIndex )
    {
        mIndexes.insert( newIndex );

        PersistedLevelRecord record;
        std::memcpy( record.magic, PersistentLevelRecordMagicValue, sizeof record.magic );
        record.majorVersion = PersistedLevelRecord::MajorVersionValue;
        record.minorVersion = PersistedLevelRecord::MinorVersionValue;
        record.level        = mLevel;
        record.count        = mSource->getCount();
        eSeek( newIndex.rec.offset, SeekRecordCode );
        mFile.write( (const char*) &record, sizeof record );

        for( int i = 0; i < record.count; ++i ) {
            unsigned char c = mSource->get();
            if ( !c ) {
                error( SourceLevelCode );
            }
            mFile.putChar( c );
        }

        // sanity check:
        if ( mSourceRecorder.getLevel() != mLevel ) {
            error( UpdateLevelSanity );
        }
    }

    int mLevel;
    Recorder& mSourceRecorder;
    RecorderSource* mSource;

    typedef enum {
        SourceLevelCode = ErrorCodeUpperBound,
        UpdateLevelSanity
    } UpdateRunnableErrorCode;
};

Persist::Persist( const char* path, QObject* parent ) : QObject(parent),
  mPath(path ? path : QDir::home().absoluteFilePath("qlt.sav")), mFileUnusable(false), mUpdateRunnable(0)
{
}

Persist::~Persist()
{
}

void Persist::init( GameRegistry* registry )
{
    // background to foreground signal connection:
    QObject::connect( this, &Persist::indexReadyQueued, this, &Persist::onIndexReadyQueued,
      Qt::ConnectionType( Qt::QueuedConnection|Qt::UniqueConnection ) /* unique allows for multiple calls within tests*/ );

    doUpdate( new InitRunnable(*this), registry );
}

void Persist::onLevelUpdated( int number )
{
    mLastUpdateTime = QTime(); // nullify
    if ( isUnusable() ) {
        std::cout << "* Persist::onLevelUpdated(" << number << ") unusable - IGNORING" << std::endl;
        return;
    }

    if ( GameRegistry* registry = getRegistry(this) ) {
        Recorder& recorder = registry->getRecorder();

        // sanity checks:
        if ( recorder.getLevel() != number ) {
            std::cout << "** Persist: ignoring updated level " << number << "; Level " << recorder.getLevel() << " recorded" << std::endl;
            return;
        }
        if ( !recorder.getAvailableCount() ) {
            std::cout << "** Persist::onLevelUpdated: recorder is empty" << std::endl;
            return;
        }
        doUpdate( new UpdateRunnable( number, *this, recorder ) );
    }
}

void Persist::setUnusable()
{
    mFileUnusable = true;
}

void Persist::onPersistentUpdateResult()
{
    mLastUpdateTime.start();
    emit indexReadyQueued();
}

void Persist::onIndexReadyQueued()
{
    if ( mUpdateRunnable ) {
        // mark existing records as unitialized
        for( auto old = mRecords.begin(); old != mRecords.end(); ++old ) {
            old->second.offset = -1;
        }

        // update the list
        for( auto it : mUpdateRunnable->mIndexes ) {
            auto old = mRecords.find( it.rec.level );
            if ( old != mRecords.end() ) {
                old->second.offset = it.rec.offset;
                if ( old->second.size != it.rec.size ) {
                    old->second.size   = it.rec.size;
                    emit levelSetComplete( it.rec.level, it.rec.size-sizeof(PersistedLevelRecord) );
                }
            } else {
                mRecords.insert( { it.rec.level, it.rec } );
                emit levelSetComplete( it.rec.level, it.rec.size-sizeof(PersistedLevelRecord) );
            }
        }

        // remove any unexpected stale records
        for( auto old = mRecords.begin(); old != mRecords.end(); ) {
            if ( old->second.offset < 0 ) {
                std::cout << "* Persist: removing stale index " << old->first << std::endl;
                mRecords.erase( old );
            } else {
                ++old;
            }
        }

        mUpdateRunnable = 0;
        mSharedRunnable.reset();
    } else {
        std::cout << "** Persist: indexReadyQeued received without UpdateRunnable" << std::endl;
    }
}

void Persist::doUpdate( PersistentUpdateRunnable* runnable, GameRegistry* registry )
{
    if ( mUpdateRunnable ) {
        std::cout << "** Persist::onLevelUpdated: Discarding new runnable - update already busy" << std::endl;
        delete runnable;
    } else {
        mUpdateRunnable = runnable;
        mSharedRunnable.reset( runnable );

        if ( !registry ) {
            registry = getRegistry(this);
        }
        registry->getWorker().doWork( mSharedRunnable );
    }
}

QString Persist::getPath() const
{
    return mPath;
}

QTime Persist::lastUpdateTime() const
{
    return mLastUpdateTime;
}

bool Persist::isPersisted( int level ) const
{
    auto it = mRecords.find( level );
    return it != mRecords.end();
}

PersistLevelLoader* Persist::getLevelLoader( int level )
{
    auto it = mRecords.find( level );
    if ( it != mRecords.end() ) {
        return new PersistLevelLoader( *this, it->second );
    }
    return 0;
}

bool Persist::isUnusable() const
{
    return mFileUnusable;
}

bool Persist::updateInProgress() const
{
    return mUpdateRunnable != 0;
}

class LoadLevelRunnable : public PersistentRunnable
{
public:
    LoadLevelRunnable( PersistLevelLoader& loader, Loadable& loadable ) : PersistentRunnable(loader.mPersist),
      mLoader(loader), mWorking(false), mLoadable(loadable)
    {
    }

    bool doLoad()
    {
        if ( !mPersist.isUnusable() ) {
            if ( GameRegistry* registry = getRegistry(&mLoader.mPersist) ) {
                mWorking = true;
                registry->getWorker().doWork( this );
            }
        }
        return mWorking;
    }

    bool isLoading()
    {
        return mWorking;
    }

    void run() override
    {
        eOpen( QIODevice::ReadOnly );
        eSeek( mLoader.mIndex.offset, SeekRecordCode );

        PersistedLevelRecord record;
        eRead( (char*) &record, sizeof record, ReadRecordCode );
        if ( std::memcmp( record.magic, PersistentLevelRecordMagicValue, sizeof record.magic ) ) {
            error( RecordMagicCode );
        }
        if ( record.majorVersion != PersistedLevelRecord::MajorVersionValue ) {
            error( RecordMajorCode );
        }
        if ( record.level != mLoader.mIndex.level ) {
            error( RecordLevelCode );
        }
        if ( record.count != mLoader.mIndex.size - (int) sizeof(PersistedLevelRecord) ) {
            error( RecordSizeCode );
        }
        if ( char* data = mLoadable.getLoadableDestination( record.level, record.count ) ) {
            int nRead = mFile.read( data, record.count );
            mLoadable.releaseLoadableDestination( record.level, nRead );
            if ( nRead != record.count ) {
                warn( QString("LoadLevelRunnable: level %1: read %2. %3 expected").arg(record.level).arg(nRead).arg(record.count) );
            }
        } else {
            warn( QString("LoadLevelRunnable: getLoadableDestination(%1,%2) returned 0").arg(record.level).arg(record.count) );
        }
    }

    void runInternal() override
    {
        PersistentRunnable::runInternal();
        mWorking = false;
        mLoader.onLoaded();
    }

    bool deleteWhenDone()
    {
        return true;
    }

private:
    PersistLevelLoader& mLoader;
    bool mWorking;
    Loadable& mLoadable;
};

PersistLevelLoader::PersistLevelLoader( Persist& persist, PersistedLevelIndex index, QObject* parent )
  : QObject(parent), mPersist(persist), mIndex(index), mStarted(false)
{
    // background to foreground signal connection:
    QObject::connect( this, &PersistLevelLoader::dataReadyInternal, this, &PersistLevelLoader::onDataReadyInternal,
      Qt::ConnectionType( Qt::QueuedConnection|Qt::UniqueConnection ) /* unique allows for multiple calls within tests*/ );
}

int PersistLevelLoader::getCount()
{
    return mIndex.size - sizeof(PersistedLevelRecord);
}

bool PersistLevelLoader::load( Loadable& loadable )
{
    if ( !mStarted ) {
        mStarted = true;
        LoadLevelRunnable* runnable = new LoadLevelRunnable( *this, loadable );
        if ( runnable->doLoad() ) {
            return true;
        }
        delete runnable;
    }
    return false;
}

void PersistLevelLoader::onDataReadyInternal()
{
    emit dataReady();
}

void PersistLevelLoader::onLoaded()
{
    emit dataReadyInternal();
}

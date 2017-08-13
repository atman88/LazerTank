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
#include "model/tank.h"

static const char PersistentIndexFooterMagicValue[3] = { 'L', 'T', 'i' };
static const char PersistentLevelRecordMagicValue[3] = { 'L', 'T', 'r' };

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
        ReadRecordCode
    } ErrorCode;

    static const int SaneMaxFileSize = 100 *
      (sizeof(PersistedLevelRecord)+Recorder::SaneMaxCapacity + sizeof(PersistedLevelIndex)) + sizeof(PersistedLevelIndexFooter);

    static const int UnusableFileSize = -2;

    PersistentRunnable( Persist& persist ) : ErrorableRunnable(true), mPersist(persist), mFile( persist.mPath )
    {
    }
    virtual ~PersistentRunnable() {}

    void warn( QString msg )
    {
        std::cout << qPrintable(mFile.fileName()) << ": " << qPrintable(msg);
        if ( mFile.error() != QFileDevice::NoError ) {
            std::cout << ": " << qPrintable(mFile.errorString()) << " (pos=" << mFile.pos() << ")";
        }
        std::cout << std::endl;
    }

    void onError( int errorCode ) override
    {
        QString msg;
        switch( errorCode ) {
        case OpenCode:        msg = "open"; mPersist.mFileSize = UnusableFileSize;                            break;
        case SeekFooterCode:  msg = "seek to footer";                                                         break;
        case FooterMagicCode: msg = "not a saved game";                                                       break;
        case ReadFooterCode:  msg = "couldn't read footer";                                                   break;
        case FooterMajorCode: msg = QString("incompatible found=%1").arg(mPersist.mFooter.majorVersion);      break;
        case SeekIndexCode:   msg = QString("couldn't seek to indexes count=%1").arg(mPersist.mFooter.count); break;
        case ReadIndexCode:   msg = "couldn't read index";                                                    break;
        default:              msg = QString("unspecified error %1").arg(errorCode);                           break;
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

protected:
    Persist& mPersist;
    QFile mFile;
};

class InitRunnable : public PersistentRunnable
{
public:
    InitRunnable( Persist& persist ) : PersistentRunnable(persist)
    {
    }

    void run() override
    {
        std::cout << "persist: running InitRunnable" << std::endl;
        if ( mFile.exists() ) {
            eOpen( QIODevice::ReadOnly );

            auto fileSize = mFile.size();
            if ( fileSize > SaneMaxFileSize ) {
                warn( QString("insane size %1").arg( fileSize ) );
                mPersist.mFileSize = 0;
            } else if ( ((unsigned) fileSize) < sizeof(PersistedLevelIndexFooter) ) {
                warn( QString("size %1 too small to read").arg( fileSize ) );
                mPersist.mFileSize = 0;
            } else {
                mPersist.mFileSize = (int) mFile.size();
                eSeek( mPersist.mFileSize-sizeof(PersistedLevelIndexFooter), SeekFooterCode );
                eRead( (char*) &mPersist.mFooter, sizeof mPersist.mFooter, ReadFooterCode );
                if ( std::memcmp( mPersist.mFooter.magic, PersistentIndexFooterMagicValue, sizeof mPersist.mFooter.magic ) ) {
                    error( FooterMagicCode );
                }
                if ( mPersist.mFooter.majorVersion != PersistedLevelIndexFooter::MajorVersionValue ) {
                    error( FooterMajorCode );
                }

                eSeek( mPersist.mFileSize - (sizeof mPersist.mFooter) - mPersist.mFooter.count * sizeof(PersistedLevelIndex), SeekIndexCode );
                for( int i = 0; i < mPersist.mFooter.count; ++i ) {
                    PersistedLevelIndex index;
                    eRead( (char*) &index, sizeof index, ReadIndexCode );
                    mPersist.onIndexRead( index );
                }
            }
        }
    }
};

class UpdateRunnable : public PersistentRunnable
{
public:
    UpdateRunnable( int level, Persist& persist, Recorder& sourceRecorder )
      : PersistentRunnable(persist), mLevel(level), mSourceRecorder(sourceRecorder), mSource(0)
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
        std::cout << "persist: running UpdateRunnable level=" << mLevel << std::endl;
        eOpen( QIODevice::WriteOnly );
        auto it = mPersist.mRecords.find( mLevel );
        if ( it == mPersist.mRecords.end() ) {
            std::cout << "** UpdateRunnable: Level << " << mLevel << " not found" << std::endl;
        } else {
            mSource = mSourceRecorder.source();

            PersistedLevelRecord record;
            std::memcpy( record.magic, PersistentLevelRecordMagicValue, sizeof record.magic );
            record.majorVersion = PersistedLevelRecord::MajorVersionValue;
            record.minorVersion = PersistedLevelRecord::MinorVersionValue;
            record.level        = mLevel;
            record.count        = mSource->getCount();
            mFile.write( (const char*) &record, sizeof record );

            bool errored = false;
            for( int i = 0; i < record.count; ++i ) {
                unsigned char c = mSource->get();
                if ( !c ) {
                    std::cout << "** UpdateRunnable: premature end during write offset=" << i << " count=" << record.count << std::endl;
                    errored = true;
                    break;
                }
                mFile.putChar( c );
            }

            // sanitize:
            if ( mSourceRecorder.getLevel() != mLevel ) {
                std::cout << "** UpdateRunnable: underlying recorder level changed? " << mLevel << " -> " << mSourceRecorder.getLevel() << std::endl;
            } else if ( !errored ) {
                // commit the index
                mFile.write( (const char*) &it->second, sizeof(PersistedLevelIndex) );
                std::memcpy( &mPersist.mFooter.magic, PersistentIndexFooterMagicValue, sizeof mPersist.mFooter.magic );
                mPersist.mFooter.majorVersion = PersistedLevelIndexFooter::MajorVersionValue;
                mPersist.mFooter.minorVersion = PersistedLevelIndexFooter::MinorVersionValue;
                mPersist.mFooter.count = 1;
                mFile.write( (const char*) &mPersist.mFooter, sizeof mPersist.mFooter );
                mFile.flush();
                mPersist.mFileSize = (int) mFile.pos();

                mPersist.mLastUpdateTime.start();
            }
            // else need to roll back mRecords here?
        }
    }

private:
    int mLevel;
    Recorder& mSourceRecorder;
    RecorderSource* mSource;
};

Persist::Persist( const char* path, QObject* parent ) : QObject(parent),
  mPath(path ? path : QDir::home().absoluteFilePath("qlt.sav")), mFileSize(0)
{
}

Persist::~Persist()
{
}

void Persist::init( GameRegistry* registry )
{
    registry->getWorker().doWork( new InitRunnable(*this) );
}

void Persist::onLevelUpdated( int number )
{
    mLastUpdateTime = QTime(); // nullify
    if ( mFileSize == PersistentRunnable::UnusableFileSize ) {
        std::cout << "* Persist::onLevelUpdated(" << number << ") UnusableFileSize - IGNORING" << std::endl;
        return;
    }

    if ( GameRegistry* registry = getRegistry(this) ) {
        Recorder& recorder = registry->getTank().getRecorder();

        // sanity check:
        if ( recorder.getLevel() != number ) {
            std::cout << "** Persist: ignoring updated level " << number << "; Level " << recorder.getLevel() << " recorded" << std::endl;
            return;
        }

        if ( int count = recorder.getCount() ) {
            PersistedLevelIndex index;
            index.level = number;
            index.offset = 0;
            index.size = sizeof(PersistedLevelRecord) + count;
            mRecords.insert( { number, index } );
            emit levelSetComplete( number );

            UpdateRunnable* runnable = new UpdateRunnable( number, *this, recorder );
            registry->getWorker().doWork( runnable );
        } else {
            std::cout << "** Persist::onLevelUpdated: recorder is empty" << std::endl;
        }
    }
}

void Persist::onIndexRead( PersistedLevelIndex index )
{
    mRecords.insert( { index.level, index } );
    emit levelSetComplete( index.level );
}

QString Persist::getPath() const
{
    return mPath;
}

QTime Persist::lastUpdateTime() const
{
    return mLastUpdateTime;
}

int Persist::getFileSize() const
{
    return mFileSize;
}

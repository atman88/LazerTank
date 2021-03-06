#include <iostream>
#include <QXmlSimpleReader>
#include <QXmlDefaultHandler>

#include "level.h"
#include "view/boardrenderer.h"
#include "controller/gameregistry.h"
#include "model/boardpool.h"
#include "util/gameutils.h"
#include "util/qltxmlhandler.h"


Level::Level( int number, int width, int height ) : mNumber(number), mSize(QSize(width,height)), mCompletedCount(0)
{
}

bool Level::operator==( const Level& other ) const
{
    return mNumber == other.mNumber;
}

bool Level::operator<( const Level& other ) const
{
    return mNumber < other.mNumber;
}

int Level::getNumber() const
{
    return mNumber;
}

const QSize& Level::getSize() const
{
    return mSize;
}

int Level::getCompletedCount() const
{
    return mCompletedCount;
}

void Level::setCompletedCount( int completedCount )
{
    mCompletedCount = completedCount;
}


class LevelXmlHandler : public QltXmlHandler
{
public:
    LevelXmlHandler( LevelList& levels ) : mLevels(levels)
    {
    }

    bool startElement(const QString& /*namespaceURI*/, const QString& localName, const QString& /*qName*/,
      const QXmlAttributes& attributes ) override
    {
        if ( localName == "level" ) {
            if ( int number = attributes.value("n").toInt() ) {
                if ( int width = attributes.value("w").toInt() ) {
                    if ( int height = attributes.value("h").toInt() ) {
                        mLevels.addLevel( number, width, height );

                        if ( width > mLevels.mVisualSizeHint.width() ) {
                            mLevels.mVisualSizeHint.setWidth( width );
                        }
                        mLevels.mVisualSizeHint.setHeight( mLevels.mVisualSizeHint.height() + height );
                    }
                }
            }
        }

        return true;
    }

    bool isDone() override
    {
        return false;
    }

private:
    LevelList& mLevels;
};


class ListLoadRunnable : public BasicRunnable
{
public:
    ListLoadRunnable( LevelList& levelList ) : mLevelList(levelList)
    {
    }

    void run() override
    {
        QFile source( ":/maps/levels.xml" );
        if ( source.open( QIODevice::ReadOnly ) ) {
            QXmlSimpleReader xml;
            LevelXmlHandler handler( mLevelList );
            xml.setContentHandler( &handler );
            QXmlInputSource xmlInputSource( &source );

            if ( !xml.parse( xmlInputSource ) ) {
                std::cout << "** error parsing " << qPrintable(source.fileName()) << ": " << qPrintable(handler.errorString()) << std::endl;
            }
        } else {
            std::cout << "** couldn't' read " << qPrintable(source.fileName()) << std::endl;
        }

        mLevelList.mInitialized = true;

        emit mLevelList.initialized();
    }

    bool deleteWhenDone() override
    {
        return true;
    }

private:
    LevelList& mLevelList;
};


LevelList::LevelList() : mInitialized{false}
{
}

void LevelList::init( GameRegistry* registry )
{
    qRegisterMetaType<Level>("Level");
    registry->getWorker().doWork( new ListLoadRunnable( *this ) );
}

void LevelList::addLevel( int number, int width, int height )
{
    mLevels.append( Level( number, width, height ) );
}

int LevelList::rowCount( const QModelIndex& ) const
{
    return size();
}

QVariant LevelList::data( const QModelIndex& index, int role ) const
{
    if ( index.row() >= 0 && index.row() < size() && (role == Qt::DisplayRole || role == Qt::EditRole) ) {
        return QVariant::fromValue( *at( index.row() ) );
    }

    return QVariant();
}

const Level* LevelList::at( int index ) const
{
    if ( 0 <= index && index < mLevels.size() ) {
        return &mLevels.at( index );
    }
    return nullptr;
}

int LevelList::numberAt( int index ) const
{
    if ( 0 <= index && index < mLevels.size() ) {
        return mLevels.at( index ).getNumber();
    }
    return 0;
}

int LevelList::indexOf( int number ) const
{
    for( int index = std::min( number, mLevels.size() ); --index >= 0; ) {
        if ( int delta = mLevels[index].getNumber() - number ) {
            if ( delta < 0 ) {
                break;
            }
        } else {
            return index;
        }
    }
    return -1;
}

const Level* LevelList::find( int number ) const
{
    int index = indexOf( number );
    if ( index >= 0 ) {
        return &mLevels.at(index);
    }
    return nullptr;
}

int LevelList::nextLevel( int curLevel ) const
{
    int index;
    if ( curLevel <= 0 ) {
        index = 0;
    } else {
        index = std::min( curLevel+1, mLevels.size() );
        while( --index >= 0 ) {
            if ( mLevels[index].getNumber() <= curLevel ) {
                ++index;
                break;
            }
        }
    }

    return numberAt( index );
}

int LevelList::size() const
{
    return mLevels.size();
}

bool LevelList::isInitialized() const
{
    return mInitialized;
}

bool LevelList::isLevelCompleted( int number ) const
{
    if ( const Level* level = find( number ) ) {
        return level->getCompletedCount() > 0;
    }
    return false;
}

int LevelList::getCompletedCount() const
{
    int count = 0;
    for( const auto& it : mLevels ) {
        if ( it.getCompletedCount() > 0 ) {
            ++count;
        }
    }
    return count;
}

void LevelList::setCompleted( int number, int moveCount )
{
    int i = indexOf( number );
    if ( i >= 0 ) {
        mLevels[i].setCompletedCount( moveCount );
        emit levelUpdated( index(i) );
    }
}

QSize LevelList::visualSizeHint() const
{
    return mVisualSizeHint;
}

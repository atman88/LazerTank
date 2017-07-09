#include <iostream>
#include <QXmlSimpleReader>
#include <QXmlDefaultHandler>

#include "level.h"
#include "view/boardrenderer.h"
#include "controller/gameregistry.h"
#include "model/boardpool.h"
#include "util/gameutils.h"


Level::Level( int number, int width, int height ) : mNumber(number), mSize(QSize(width,height))
{
}

Level::Level( const Level& other) : mNumber(other.mNumber), mSize(other.mSize)
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


class LevelXmlHandler : public QXmlDefaultHandler
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

private:
    LevelList& mLevels;
};


class ListLoadRunnable : public Runnable
{
public:
    ListLoadRunnable( LevelList& levelList ) : Runnable(true), mLevelList(levelList)
    {
    }

    ~ListLoadRunnable()
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
                std::cout << "** error parsing maps/levels.xml: " << qPrintable(xml.errorHandler()->errorString()) << std::endl;
            }
        } else {
            std::cout << "** couldn't' read maps/levels.xml" << std::endl;
        }

        mLevelList.mInitialized = true;

        emit mLevelList.initialized();
    }

private:
    LevelList& mLevelList;
};


LevelList::LevelList() : mInitialized(false)
{
    qRegisterMetaType<Level>("Level");
}

void LevelList::init( GameRegistry* registry )
{
    registry->getWorker().doWork( new ListLoadRunnable( *this ) );
}

void LevelList::addLevel( int number, int width, int height )
{
    Level* level = new Level( number, width, height );
    mLevels.append( level );
}

Level* LevelList::at( int index ) const
{
    if ( 0 <= index && index < mLevels.size() ) {
        return mLevels.at( index );
    }
    return 0;
}

int LevelList::numberAt( int index ) const
{
    if ( 0 <= index && index < mLevels.size() ) {
        if ( Level* level = mLevels.at( index ) ) {
            return level->getNumber();
        }
    }
    return 0;
}

int LevelList::indexOf( int number ) const
{
    for( int index = std::min( number, mLevels.size() ); --index >= 0; ) {
        if ( int delta = mLevels[index]->getNumber() - number ) {
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
        return mLevels.at(index);
    }
    return 0;
}

int LevelList::nextLevel( int curLevel ) const
{
    int index;
    if ( curLevel <= 0 ) {
        index = 0;
    } else {
        index = std::min( curLevel+1, mLevels.size() );
        while( --index >= 0 ) {
            if ( mLevels[index]->getNumber() <= curLevel ) {
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

QSize LevelList::visualSizeHint() const
{
    return mVisualSizeHint;
}

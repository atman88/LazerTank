#include <iostream>
#include <QProxyStyle>
#include <QStyleOption>
#include <QSize>
#include <QScreen>
#include <QXmlSimpleReader>
#include <QXmlDefaultHandler>

#include "levelchooser.h"
#include "boardrenderer.h"
#include "boardwindow.h"
#include "controller/gameregistry.h"
#include "controller/game.h"
#include "model/level.h"
#include "model/board.h"
#include "model/boardpool.h"

#define TILE_SIZE 12

class ChooserStyle : public QProxyStyle
{
public:
    ChooserStyle( QSize defaultItemSize ) : mDefaultItemSize(defaultItemSize), mPalette(QPalette(Qt::gray,Qt::black))
    {
    }

    QPalette standardPalette() const override
    {
        return mPalette;
    }

    int styleHint(StyleHint hint, const QStyleOption *option = 0, const QWidget *widget = 0, QStyleHintReturn *returnData = 0)
      const
    {
        switch( hint ) {
        case QStyle::SH_Menu_Scrollable:
            return 1;
        case QStyle::SH_Menu_SelectionWrap:
            // disable wrapping so the visible area stays contiguous (keeps the BoardPool logic simple)
            return 0;
        default:
            return QProxyStyle::styleHint(hint, option, widget, returnData);
        }
    }

    QSize sizeFromContents( ContentsType type, const QStyleOption* option, const QSize& size, const QWidget* widget ) const
    {
        if ( type == CT_MenuItem ) {
            if ( const LevelChooser* chooser = qobject_cast<const LevelChooser*>(widget) ) {
                if ( const QStyleOptionMenuItem *menuOption = qstyleoption_cast<const QStyleOptionMenuItem*>( option ) ) {
                    bool ok;
                    int number = menuOption->text.toInt( &ok );
                    if ( ok ) {
                        if ( Level* level = chooser->find( number ) ) {
                            return level->getSize();
                        }
                    }
                    return mDefaultItemSize;
                }
            }
        }

        return QProxyStyle::sizeFromContents( type, option, size, widget );
    }

private:
    QSize mDefaultItemSize;
    QPalette mPalette;
};

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

                        if ( mLevels.mMinSize.isNull() ) {
                            mLevels.mMinSize = mLevels.mMaxSize = QSize( width, height );
                        } else {
                            if      ( width  > mLevels.mMaxSize.width()  ) mLevels.mMaxSize.setWidth(  width  );
                            else if ( width  < mLevels.mMinSize.width()  ) mLevels.mMinSize.setWidth(  width  );
                            if      ( height > mLevels.mMaxSize.height() ) mLevels.mMaxSize.setHeight( height );
                            else if ( height < mLevels.mMinSize.height() ) mLevels.mMinSize.setHeight( height );
                        }
                    }
                }
            }
        }

        return true;
    }

private:
    LevelList& mLevels;
};

LevelList::LevelList() : mInitialized(false)
{
}

void LevelList::addLevel( int number, int width, int height )
{
    Level* level = new Level( number, width, height );
    level->moveToThread( thread() );
    mLevels.append( level );
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

Level* LevelList::find( int number ) const
{
    for( int index = std::min( number, mLevels.size() ); --index >= 0; ) {
        if ( int delta = mLevels[index]->getNumber() - number ) {
            if ( delta < 0 ) {
                break;
            }
        } else {
            return mLevels.at(index);
        }
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

LevelChooser::LevelChooser( QWidget* parent ) : QMenu(parent), mRealized(false)
{
    setStyle( new ChooserStyle( QSize( mLevelList.mMaxSize.width(), mLevelList.mMinSize.height() ) * TILE_SIZE ) );
}

void LevelChooser::init( GameRegistry* registry )
{
    QObject::connect( &mLevelList, &LevelList::initialized, this, &LevelChooser::onLevelListInitialized, Qt::QueuedConnection );
    registry->getWorker().doWork( new ListLoadRunnable( this->mLevelList ) );
}

void LevelChooser::realize()
{
    if ( !mRealized && mLevelList.mInitialized ) {
        if ( GameRegistry* registry = getRegistry(this) ) {
            for( Level* level : mLevelList.mLevels ) {
                LevelWidget* widget = new LevelWidget( *level );
                level->setDefaultWidget( widget );
                addAction( qobject_cast<QAction*>(level) );
            }
            BoardPool& pool = registry->getBoardPool();
            QObject::connect( &pool, &BoardPool::boardLoaded, this, &LevelChooser::onBoardLoaded );
            mRealized = true;
        }
    }
}

Level* LevelChooser::find( int number ) const
{
    return mLevelList.find( number );
}

bool LevelChooser::isListInitialized() const
{
    return mLevelList.mInitialized;
}

bool LevelChooser::isRealized() const
{
    return mRealized;
}

int LevelChooser::nextLevel( int curLevel ) const
{
    return mLevelList.nextLevel( curLevel );
}

LevelList& LevelChooser::getList()
{
    return mLevelList;
}

void LevelChooser::onLevelListInitialized()
{
    // initialize the pool
    if ( GameRegistry* registry = getRegistry(this) ) {
        if ( BoardWindow* window = registry->getWindow() ) {
            if ( QScreen* screen = window->screen() ) {
                QSize maxSize = screen->availableSize();
                registry->getBoardPool().init( mLevelList, maxSize.height() / TILE_SIZE );
            }
        }
    }

    emit listInitialized();
}

void LevelChooser::onBoardLoaded( int number )
{
    if ( Level* level = find( number ) ) {
        level->onBoardLoaded();
    }
}

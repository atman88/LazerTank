#include <iostream>
#include <QXmlSimpleReader>
#include <QXmlDefaultHandler>
#include <QPainter>

#include "level.h"
#include "view/boardrenderer.h"
#include "controller/gameregistry.h"
#include "model/boardpool.h"
#include "util/gameutils.h"

#define TILE_SIZE 12
#define FONT_SIZE 15

#define PADDING_WIDTH  3
#define PADDING_HEIGHT 3


LevelWidget::LevelWidget( Level& source, QWidget* parent ) : QWidget(parent), mNumber(source.getNumber()),
  mBoardPixelSize( source.getSize()*TILE_SIZE )
{
    setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed );
    setFocusPolicy( Qt::StrongFocus );
    setAutoFillBackground( true );
}

QSize LevelWidget::sizeHint() const
{
    return mBoardPixelSize + QSize(PADDING_WIDTH*2,PADDING_HEIGHT*2);
}

void LevelWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize( FONT_SIZE );
    painter.setFont( font );

    if ( hasFocus() ) {
        painter.fillRect( event->rect(), QColor(0,255,33) );
    }

    QRect myRect( rect() );
    if ( GameRegistry* registry = getRegistry(parent()) ) {
        if ( Board* board = registry->getBoardPool().getBoard( mNumber ) ) {
            BoardRenderer renderer( TILE_SIZE );
            QPoint offset( std::max( (myRect.width()  - mBoardPixelSize.width() )/2, PADDING_WIDTH  ),
                           std::max( (myRect.height() - mBoardPixelSize.height())/2, PADDING_HEIGHT ) );
            painter.translate( offset );
            renderer.render( &myRect, board, &painter );
            renderer.renderInitialTank( board, &painter );
            painter.resetTransform();
        }
    }

    myRect -= QMargins(PADDING_WIDTH, PADDING_HEIGHT, PADDING_WIDTH, PADDING_HEIGHT );
    painter.drawText( myRect, Qt::AlignBottom|Qt::AlignRight|Qt::TextDontClip|Qt::TextSingleLine, QString::number(mNumber) );
}

Level::Level(int number, int width, int height, QObject* parent ) : QWidgetAction(parent), mNumber(number),
  mSize(QSize(width,height))
{
    QAction::setText( QString::number( number ) );
    QAction::setData( QVariant(number) );
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

void Level::onBoardLoaded() const
{
    if ( QWidget* widget = defaultWidget() ) {
        widget->update();
    }
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
}

void LevelList::init( GameRegistry* registry )
{
    registry->getWorker().doWork( new ListLoadRunnable( *this ) );
}

void LevelList::addLevel( int number, int width, int height )
{
    Level* level = new Level( number, width, height );
    level->moveToThread( thread() );
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

int LevelList::rowCount( const QModelIndex& ) const
{
    return mLevels.size();
}

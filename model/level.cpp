#include <iostream>
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

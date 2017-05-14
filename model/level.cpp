#include "level.h"
#include "view/boardrenderer.h"
#include "controller/gameregistry.h"
#include "model/boardpool.h"
#include "util/gameutils.h"

#define TILE_SIZE 12

LevelWidget::LevelWidget( Level& source, QWidget* parent ) : QWidget(parent), mNumber(source.getNumber()),
  mSize( source.getSize()*TILE_SIZE )
{
    setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed );
    setFocusPolicy( Qt::StrongFocus );
    setAutoFillBackground( true );
}

QSize LevelWidget::sizeHint() const
{
    return mSize;
}

void LevelWidget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);

    if ( hasFocus() ) {
        painter.fillRect( event->rect(), QColor(0,255,33) );
        painter.setPen(QColor(Qt::white));
    }

    if ( GameRegistry* registry = getRegistry(parent()) ) {
        if ( Board* board = registry->getBoardPool().getBoard( mNumber ) ) {
            BoardRenderer renderer( TILE_SIZE );
            renderer.render( &event->rect(), board, &painter );
            renderer.renderInitialTank( board, &painter );
        }
    }

    painter.drawText( rect(), Qt::AlignBottom|Qt::AlignRight|Qt::TextDontClip|Qt::TextSingleLine, QString::number(mNumber) );
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

void Level::onBoardLoaded()
{
    if ( QWidget* widget = defaultWidget() ) {
        widget->update();
    }
}

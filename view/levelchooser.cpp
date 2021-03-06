#include <QSize>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QStyle>
#include <QApplication>

#include "levelchooser.h"
#include "boardrenderer.h"
#include "model/board.h"
#include "model/boardpool.h"
#include "util/imageutils.h"

#define PADDING_WIDTH  3
#define PADDING_HEIGHT 3

class LevelPainter : public QStyledItemDelegate
{
public:
    explicit LevelPainter( BoardPool& pool, QObject* parent = nullptr ) : QStyledItemDelegate(parent), mPool(pool)
    {
    }

    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const override
    {
        painter->save();

        QFont font = painter->font();
        painter->setFont( font );

        if ( option.state & QStyle::State_Selected ) {
            painter->fillRect( option.rect, option.palette.highlight() );
        }

        painter->translate( option.rect.topLeft() );
        QRect rect( 0, 0, option.rect.width(), option.rect.height() );

        Level level = qvariant_cast<Level>( index.model()->data( index ) );
        if ( Board* board = mPool.getBoard( level.getNumber() ) ) {
            BoardRenderer renderer( ChooserTileSize );
            QPoint offset( std::max( (rect.width()  - board->getWidth() *ChooserTileSize)/2, PADDING_WIDTH  ),
                           std::max( (rect.height() - board->getHeight()*ChooserTileSize)/2, PADDING_HEIGHT ) );
            painter->translate( offset );
            renderer.render( rect, board, painter );
            renderer.renderInitialTank( board, painter );
            painter->translate( -offset );
        }

        painter->drawText( rect - QMargins( PADDING_WIDTH, PADDING_HEIGHT, PADDING_WIDTH, PADDING_HEIGHT ),
          Qt::AlignBottom|Qt::AlignRight|Qt::TextDontClip|Qt::TextSingleLine, QString::number(level.getNumber()) );

        if ( level.getCompletedCount() > 0 ) {
            painter->drawPixmap( PADDING_WIDTH, 0, *ResourcePixmap::getPixmap(COMPLETE_CHECKMARK) );
        }

        painter->restore();
    }

    QSize sizeHint( const QStyleOptionViewItem& /*option*/, const QModelIndex& index ) const override
    {
        return qvariant_cast<Level>( index.model()->data( index ) ).getSize()*ChooserTileSize + QSize(PADDING_WIDTH*2,PADDING_HEIGHT*2);
    }

private:
    BoardPool& mPool;
};

LevelChooser::LevelChooser( LevelList& levels, BoardPool& pool, QWidget* parent ) : QListView(parent)
{
    setWindowFlags( Qt::Dialog );
    setWindowTitle( "Select Level" );
    setWindowModality( Qt::ApplicationModal );
    setStyleSheet(
      "QListView {"
        "background-color: black;"
        "color: white;"
        "selection-background-color: rgb(0,255,33);"
        "font: bold 15px;"
      "}"
    );

    setModel( &levels );

    setItemDelegate( new LevelPainter( pool, this ) );

    QObject::connect( this, &LevelChooser::activated, this, &LevelChooser::onActivated );
    QObject::connect( &pool, &BoardPool::boardLoaded, this, &LevelChooser::onBoardLoaded );
    QObject::connect( &levels, SIGNAL(levelUpdated(const QModelIndex&)), this, SLOT(update(const QModelIndex&)), Qt::QueuedConnection );
}

void LevelChooser::onActivated( const QModelIndex& index )
{
    QVariant v = index.model()->data( index );
    if ( int number = qvariant_cast<Level>(v).getNumber() ) {
        emit levelChosen( number );
    }

    close();
}

void LevelChooser::onBoardLoaded( int number )
{
    if ( const LevelList* list = dynamic_cast<LevelList*>( model() ) ) {
        QModelIndex index = model()->index( list->indexOf(number), 0 );
        dataChanged( index, index );
    }
}

QSize LevelChooser::preferredSize() const
{
    if ( const LevelList* list = dynamic_cast<LevelList*>( model() ) ) {
        return list->visualSizeHint() * ChooserTileSize
          + QSize(PADDING_WIDTH*2 + style()->pixelMetric(QStyle::PM_SliderThickness), list->size()*PADDING_HEIGHT*2 );
    }
    return QListView::viewportSizeHint();
}

void LevelChooser::setSelectedLevel( int number )
{
    if ( const LevelList* list = dynamic_cast<LevelList*>( model() ) ) {
        QModelIndex index = model()->index( list->indexOf(number), 0 );
        setCurrentIndex( index );
        scrollTo( index, EnsureVisible );
    }
}

void LevelChooser::keyPressEvent(QKeyEvent* event)
{
    // Unfortunately QAbstractItemView loses QWidget's cancel handing, so adding it back here:
    if ( event->matches(QKeySequence::Cancel) ) {
        event->accept();
        close();
    } else {
        QListView::keyPressEvent( event );
    }
}

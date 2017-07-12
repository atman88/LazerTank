#include <QSize>
#include <QPainter>
#include <QStyledItemDelegate>

#include "levelchooser.h"
#include "boardrenderer.h"
#include "model/board.h"
#include "model/boardpool.h"

#define TILE_SIZE 12
#define PADDING_WIDTH  3
#define PADDING_HEIGHT 3

class LevelModel : public QAbstractListModel
{
public:
    LevelModel( LevelList& list, QObject* parent = 0 ) : QAbstractListModel(parent), mList(list)
    {
    }

    int rowCount( const QModelIndex& ) const
    {
        return mList.size();
    }

    QVariant data( const QModelIndex& index, int role ) const
    {
        if ( index.row() >= 0 && index.row() < mList.size() && (role == Qt::DisplayRole || role == Qt::EditRole) ) {
            return QVariant::fromValue( *mList.at( index.row() ) );
        }

        return QVariant();
    }

    LevelList& getList()
    {
        return mList;
    }

private:
    LevelList& mList;
};

class LevelPainter : public QStyledItemDelegate
{
public:
    explicit LevelPainter( BoardPool& pool, QObject* parent = 0 ) : QStyledItemDelegate(parent), mPool(pool)
    {
    }

    void paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
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
            BoardRenderer renderer( TILE_SIZE );
            QPoint offset( std::max( (rect.width()  - board->getWidth() *TILE_SIZE)/2, PADDING_WIDTH  ),
                           std::max( (rect.height() - board->getHeight()*TILE_SIZE)/2, PADDING_HEIGHT ) );
            painter->translate( offset );
            renderer.render( &rect, board, painter );
            renderer.renderInitialTank( board, painter );
            painter->translate( -offset );
        }

        painter->drawText( rect - QMargins( PADDING_WIDTH, PADDING_HEIGHT, PADDING_WIDTH, PADDING_HEIGHT ),
          Qt::AlignBottom|Qt::AlignRight|Qt::TextDontClip|Qt::TextSingleLine, QString::number(level.getNumber()) );

        painter->restore();
    }

    QSize sizeHint( const QStyleOptionViewItem& /*option*/, const QModelIndex& index ) const
    {
        return qvariant_cast<Level>( index.model()->data( index ) ).getSize()*TILE_SIZE + QSize(PADDING_WIDTH*2,PADDING_HEIGHT*2);
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

    LevelModel* model = new LevelModel( levels, this );
    setModel( model );

    LevelPainter* delegate = new LevelPainter( pool, this );
    setItemDelegate( delegate );

    QObject::connect( this, &LevelChooser::activated, this, &LevelChooser::onActivated );
    QObject::connect( &pool, &BoardPool::boardLoaded, this, &LevelChooser::onBoardLoaded );
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
    if ( const LevelList* list = getList() ) {
        QModelIndex index = model()->index( list->indexOf(number), 0 );
        dataChanged( index, index );
    }
}

QSize LevelChooser::preferredSize() const
{
    if ( const LevelList* list = getList() ) {
        return list->visualSizeHint() * TILE_SIZE
          + QSize(PADDING_WIDTH*2 + style()->pixelMetric(QStyle::PM_SliderThickness), list->size()*PADDING_HEIGHT*2 );
    }
    return QListView::viewportSizeHint();
}

const LevelList* LevelChooser::getList() const
{
    if ( LevelModel* levelModel = dynamic_cast<LevelModel*>( model() ) ) {
        return &levelModel->getList();
    }
    return 0;
}

void LevelChooser::setSelectedLevel( int number )
{
    if ( const LevelList* list = getList() ) {
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

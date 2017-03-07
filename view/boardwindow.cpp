#include <iostream>
#include <QMenu>
#include <QAction>
#include <QMessageBox>

#include "boardwindow.h"
#include "util/renderutils.h"
#include "util/imageutils.h"
#include "controller/game.h"

using namespace std;

BoardWindow::BoardWindow(QWindow *parent) : QWindow(parent)
{
    mPen.setWidth(2);

    create();

    mBackingStore = new QBackingStore(this);
}

void BoardWindow::exposeEvent(QExposeEvent *)
{
    if ( mGame && isExposed() ) {
        Board* board = mGame->getBoard();
        if (board ) {
            QRect size( 0, 0, board->getWidth()*24, board->getHeight()*24 );
            mDirtyRegion += size;
            renderNow();
        }
    }
}

void BoardWindow::init( Game* game )
{
    if ( game ) {
        mGame = game;

        onBoardLoaded();
        Board* board = game->getBoard();
        QObject::connect( board, &Board::boardLoaded,   this, &BoardWindow::onBoardLoaded     );
        QObject::connect( board, &Board::tileChangedAt, this, &BoardWindow::renderSquareLater );

        PieceSetManager* pm = board->getPieceManager();
        QObject::connect( pm, &PieceSetManager::erasedAt,   this, &BoardWindow::renderSquareLater );
        QObject::connect( pm, &PieceSetManager::insertedAt, this, &BoardWindow::renderSquareLater );
    }
}

void BoardWindow::onBoardLoaded()
{
    if ( mGame ) {
        Board* board = mGame->getBoard();
        QRect size( 0, 0, board->getWidth()*24, board->getHeight()*24 );
        setGeometry(size);
        mDirtyRegion += size;

        renderLater( size );

        int level = board->getLevel();
        if ( level > 0 ) {
            QString title( QString("Level %1").arg(level) );
            setTitle( title );
        }
    }
}

void BoardWindow::resizeEvent(QResizeEvent *resizeEvent)
{
    mBackingStore->resize(resizeEvent->size());
    if (isExposed()) {
        Game* mGame = getGame(this);
        if ( mGame ) {
            Board* board = mGame->getBoard();
            QRect size( 0, 0, board->getWidth()*24, board->getHeight()*24 );
            mDirtyRegion += size;
            renderNow();
        }
    }
}

void BoardWindow::renderNow()
{
    if ( isExposed() ) {
        mDirtyRegion.swap( mRenderRegion );
        mDirtyRegion.setRects(0,0);
        render( &mRenderRegion );
    }
}

void BoardWindow::renderSquareLater( int col, int row )
{
    QRect dirty(col*24, row*24, 24, 24);
    renderLater( dirty );
}

void BoardWindow::renderListIn(PieceSet::iterator iterator, PieceSet::iterator end, const QRect* dirty, QPainter* painter )
{
    QRect bounds;
    while( iterator != end ) {
        (*iterator)->getBounds( &bounds );
        if ( dirty->intersects( bounds ) ) {
            renderPiece( (*iterator)->getType(), bounds.left(), bounds.top(), (*iterator)->getAngle(), painter );
        } else if ( bounds.top() >= dirty->bottom() ) {
            break;
        }
        ++iterator;
    }
}

QMenu& BoardWindow::getMenu()
{
    return mMenu;
}

void BoardWindow::renderOneRect( const QRect* rect, Board* board, const PieceMultiSet* moves, const PieceSet* tiles,
  const PieceSet* deltas, QPainter* painter )
{
    int minX = rect->left()/24;
    int minY = rect->top() /24;
    int maxX = (rect->right() -1)/24;
    int maxY = (rect->bottom()-1)/24;

    SimplePiece pos(MOVE, minX, minY);
    PieceSet::iterator moveIterator = moves->lower_bound( &pos );
    PieceSet::iterator tileIterator = tiles->lower_bound( &pos );
    PieceSet::iterator deltasIterator;
    if ( deltas ) {
        deltasIterator = deltas->lower_bound( &pos );
    }

    for( int y = minY; y <= maxY; ++y ) {
        for( int x = minX; x <= maxX; ++x ) {
            TileType type = board->tileAt( x, y );
            const QPixmap* pixmap = getPixmap( type );
            if ( !pixmap->isNull() ) {
                painter->drawPixmap( x*24, y*24, *pixmap );
            } else {
                int angle = 0;
                int wx = x*24;
                int wy = y*24;
                switch( type ) {
                case WATER:
                    painter->fillRect(wx, wy, 24, 24, QColor(33,33,255));
                    break;
                case STONE_MIRROR__90:
                    pixmap = getPixmap( STONE_MIRROR );
                    angle = 90;
                    break;
                case STONE_MIRROR_180:
                    pixmap = getPixmap( STONE_MIRROR );
                    angle = 180;
                    break;
                case STONE_MIRROR_270:
                    pixmap = getPixmap( STONE_MIRROR );
                    angle = 270;
                    break;
                case STONE_SLIT_90:
                    pixmap = getPixmap( STONE_SLIT );
                    angle = 90;
                    break;
                case WOOD_DAMAGED:
                    drawPixmap( wx, wy, WOOD,   painter );
                    drawPixmap( wx, wy, DAMAGE, painter );
                    break;
                default: // EMPTY
                    painter->fillRect(wx, wy, 24, 24, Qt::black);
                    break;
                }
                if ( angle && pixmap ) {
                    renderRotatedPixmap( pixmap, wx, wy, angle, painter);
                }
            }
        }
    }

    renderListIn( tileIterator, tiles->end(), rect, painter );
    if ( deltas ) {
        renderListIn( deltasIterator, deltas->end(), rect, painter );
    }
    renderListIn( moveIterator, moves->end(), rect, painter );

    if ( mGame ) {
        mGame->getMovingPiece().render( rect, painter );
        mGame->getTank()->render( rect, painter );
        mGame->getCannonShot().render( painter );
    }
}

void BoardWindow::render(QRegion* region)
{
    Board* board = mGame->getBoard();

    const PieceMultiSet* moves = mGame->getTank()->getMoves()->toMultiSet();
    const PieceSet* tiles = board->getPieceManager()->getPieces();
    const PieceSet* deltas = mGame->getDeltaPieces();

    QVector<QRect> rects = region->rects();
    int i = rects.size();
    if ( i <= 0 ) {
        return;
    }

    int minX = rects[0].left();
    int minY = rects[0].top();
    int maxX = rects[0].right();
    int maxY = rects[0].bottom();
    while( --i > 0 ) {
        minX = min( minX, rects[i].left()   );
        minY = min( minY, rects[i].top()    );
        maxX = max( maxX, rects[i].right()  );
        maxY = max( maxY, rects[i].bottom() );
    }

    QRect rect( minX, minY, maxX-minX+1, maxY-minY+1 );
    QRegion r( rect );
    mBackingStore->beginPaint(r);
    QPaintDevice *device = mBackingStore->paintDevice();
    QPainter painter(device);

    renderOneRect( &rect, board, moves, tiles, deltas, &painter );

    mBackingStore->endPaint();
    mBackingStore->flush(r);
}

void BoardWindow::renderLater( const QRect& rect )
{
    mDirtyRegion += rect;
    requestUpdate();
}

bool BoardWindow::event(QEvent *event)
{
    if (event->type() == QEvent::UpdateRequest) {
        renderNow();
        return true;
    }

    return QWindow::event(event);
}

int keyToAngle( int key )
{
    switch( key ) {
    case Qt::Key_Up:    return   0;
    case Qt::Key_Right: return  90;
    case Qt::Key_Down:  return 180;
    case Qt::Key_Left:  return 270;
    default:            return  -1;
    }
}

void BoardWindow::showMenu( QPoint* globalPos )
{
    if ( mGame ) {
        if ( mMenu.isEmpty() ) {
            QKeySequence seq(QString(" "));
            mMenu.addAction( "shoot", mGame->getTank(), &Tank::fire, seq );

            mReloadAction = mMenu.addAction( "Restart Level" );;

            QAction* action = mMenu.addAction( QString("level..") );
            action->setMenu( &mLevelsMenu );
            QString text( "level %1" );
            for( int level = 1; level <= BOARD_MAX_LEVEL; ++level ) {
                action = mLevelsMenu.addAction( text.arg(level) );
                action->setData( QVariant(level) );
            }
        }

        mReloadAction->setData( QVariant(mGame->getBoard()->getLevel()) );

        QAction* action = (globalPos ? mMenu.exec( *globalPos ) : mMenu.exec());
        if ( action ) {
            cout << "menu " << action->property("text").toString().toStdString() << " selected" << std::endl;
            bool ok;
            int level = action->data().toInt( &ok );
            if ( ok && level > 0 ) {
                mGame->getBoard()->load( action->data().toInt() );
            }
        }
    }
}

void BoardWindow::keyPressEvent(QKeyEvent *ev)
{
    if ( !ev->isAutoRepeat() && mGame ) {
        switch( ev->key() ) {
        case Qt::Key_Escape:
            showMenu();
            break;

        case Qt::Key_Space:
            mGame->getTank()->fire();
            break;

        case Qt::Key_Shift:
            emit setSpeed(HIGH_SPEED);
            break;

        case Qt::Key_C: // attempt to capture the flag
        {   Board* board = mGame->getBoard();
            Tank* tank = mGame->getTank();
            tank->stop();
            mGame->findPath( board->getFlagX(), board->getFlagY(), tank->getRotation().toInt() );
            break;
        }
        default:
            int rotation = keyToAngle(ev->key());
            if ( rotation >= 0 ) {
                mGame->getTank()->move( rotation );
            }
        }
    }
}

void BoardWindow::keyReleaseEvent(QKeyEvent *ev)
{
    if ( !ev->isAutoRepeat() && mGame ) {
        switch( ev->key() ) {
        case Qt::Key_Space:
            mGame->getTank()->ceaseFire();
            break;

        case Qt::Key_Shift:
            emit setSpeed(LOW_SPEED);
            break;

        case Qt::Key_C:
                mGame->getTank()->move();
                break;

        case Qt::Key_Backspace:
        {   PieceListManager* moveManager = mGame->getTank()->getMoves();
            Piece* piece = moveManager->getList()->back();
            if ( piece ) {
                if ( piece->hasPush() ) {
                    mGame->undoFuturePush( piece );
                }
                mGame->getTank()->getMoves()->eraseBack();
            }
            break;
        }
        default:
            ;
        }
    }
}

void BoardWindow::mousePressEvent( QMouseEvent* event )
{
    if ( !mGame ) {
        return;
    }

    switch( event->button() ) {
    case Qt::RightButton:
    {   QPoint globalPos = event->globalPos();
        showMenu( &globalPos );
        break;
    }
    case Qt::LeftButton:
    {   Tank* tank = mGame->getTank();
        tank->stop();
        mGame->findPath( event->pos().x()/24, event->pos().y()/24, tank->getRotation().toInt() );
        break;
    }
    default:
        ;
    }
}

void BoardWindow::mouseReleaseEvent( QMouseEvent* event )
{
    if ( !mGame ) {
        return;
    }

    switch( event->button() ) {
    case Qt::LeftButton:
        mGame->getTank()->move();
        break;
    default:
        ;
    }
}

void BoardWindow::onTankKilled()
{
    QMessageBox msgBox;
    msgBox.setText("Level lost!");
    msgBox.exec();

    if ( mGame ) {
        Board* board = mGame->getBoard();
        if ( board ) {
            board->load( board->getLevel() );
        }
    }
}

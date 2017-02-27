#include <iostream>
#include <QMenu>
#include <QAction>
#include "BoardWindow.h"

#include "util/imageutils.h"

BoardWindow::BoardWindow(QWindow *parent) : QWindow(parent)
{
    create();

    mBackingStore = new QBackingStore(this);

    mTank = new Tank();
    mTank->setParent(this);
    QObject::connect( mTank, &Tank::changed,    this, &BoardWindow::renderLater       );
    QObject::connect( &mTank->getMoves(), &PieceListManager::appended, this, &BoardWindow::renderSquareLater );
    QObject::connect( &mTank->getMoves(), &PieceListManager::erased,   this, &BoardWindow::renderSquareLater );
    QObject::connect( &mTank->getMoves(), &PieceListManager::replaced, this, &BoardWindow::renderSquareLater );

    mShot = new Shot(mTank);
    QObject::connect( &mShot->getPath(), &PieceListManager::appended, this, &BoardWindow::renderSquareLater );
    QObject::connect( &mShot->getPath(), &PieceListManager::erased,   this, &BoardWindow::renderSquareLater );

    mActiveMoveDirection = -1;
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

GameHandle BoardWindow::getGame() const
{
    return mGame->getHandle();
}

void BoardWindow::setGame(const GameHandle handle )
{
    setProperty("GameHandle", QVariant::fromValue(handle));

    mGame = handle.game;
    if ( mGame ) {
        QObject::connect( &mGame->getMovingPiece(),&Push::pieceMoved,   this,  &BoardWindow::renderLater      );
        QObject::connect( mTank,                   &Tank::movingInto,   mGame, &Game::onTankMovingInto        );
        QObject::connect( mTank,                   &Tank::moved,        mGame, &Game::onTankMoved             );

        Board* board = mGame->getBoard();
        if ( board ) {
            onBoardLoaded();
            QObject::connect( board, &Board::boardLoaded,   this, &BoardWindow::onBoardLoaded     );
            QObject::connect( board, &Board::tileChangedAt, this, &BoardWindow::renderSquareLater );
            PieceSetManager& pm = board->getPieceManager();
            QObject::connect( &pm, &PieceSetManager::erasingAt,  this, &BoardWindow::renderSquareLater );
            QObject::connect( &pm, &PieceSetManager::insertedAt, this, &BoardWindow::renderSquareLater );
        }
        mTank->init( mGame );
        mShot->init( mGame->getShotAggregate() );
    }
}

void BoardWindow::onBoardLoaded()
{
    Board* board = mGame->getBoard();
    if ( board ) {
        QRect size( 0, 0, board->getWidth()*24, board->getHeight()*24 );
        setGeometry(size);
        mDirtyRegion += size;
        mTank->reset( board->mInitialTankX, board->mInitialTankY );

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
    if (isExposed() && mGame) {
        Board* board = mGame->getBoard();
        if ( board ) {
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

void BoardWindow::renderRotatedPixmap( const QPixmap* pixmap, int x, int y, int angle, QPainter* painter )
{
    if ( angle ) {
        int centerX = x + 24/2;
        int centerY = y + 24/2;
        painter->translate(centerX, centerY);
        painter->rotate(angle);
        painter->translate(-centerX, -centerY);
    }
    painter->drawPixmap( x, y, *pixmap );
    painter->resetTransform();
}

void BoardWindow::renderPiece( PieceType type, int x, int y, int angle, QPainter* painter )
{
    const QPixmap* pixmap = getPixmap( type );

    switch( type ) {
    case SHOT_RIGHT:
        angle = (angle + 270) % 360;
        break;
    case SHOT_LEFT:
        pixmap = getPixmap( SHOT_RIGHT );
        angle = (angle + 180) % 360;
        break;
    default:
        break;
    }

    if ( pixmap->isNull() ) {
        cout << "no pixmap for " << type << std::endl;
        return;
    }

    renderRotatedPixmap( pixmap, x, y, angle, painter );
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

void BoardWindow::renderOneRect( const QRect* rect, Board* board, const PieceSet* moves, const PieceSet* tiles,
  const PieceSet* deltas, const PieceSet* shots, QPainter* painter )
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
    PieceSet::iterator shotIterator = shots->lower_bound( &pos );

    for( int y = minY; y <= maxY; ++y ) {
        for( int x = minX; x <= maxX; ++x ) {
            TileType type = board->tileAt( x, y );
            const QPixmap* pixmap = getPixmap( type );
            if ( !pixmap->isNull() ) {
                painter->drawPixmap( x*24, y*24, *pixmap );
            } else {
                int angle = 0;
                switch( type ) {
                case WATER:
                    painter->fillRect(x*24, y*24, 24, 24, Qt::blue);
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
                default: // EMPTY
                    painter->fillRect(x*24, y*24, 24, 24, Qt::black);
                    break;
                }
                if ( angle ) {
                    renderRotatedPixmap( pixmap, x*24, y*24, angle, painter);
                }
            }
        }
    }

    renderListIn( tileIterator, tiles->end(), rect, painter );
    if ( deltas ) {
        renderListIn( deltasIterator, deltas->end(), rect, painter );
    }
    renderListIn( moveIterator, moves->end(), rect, painter );

    Push& movingPiece = mGame->getMovingPiece();
    if ( movingPiece.getType() != NONE ) {
        QRect* bounds = movingPiece.getBounds();
        if ( rect->intersects( *bounds ) ) {
            renderPiece( movingPiece.getType(), bounds->left(), bounds->top(), movingPiece.getPieceAngle(), painter );
        }
    }

    if ( rect->intersects( mTank->getRect() ) ) {
        mTank->paint( painter );
    }

    renderListIn( shotIterator, shots->end(), rect, painter );
}

void BoardWindow::render(QRegion* region)
{
    if ( !mGame ) {
        return;
    }
    Board* board = mGame->getBoard();
    if ( !board ) {
        return;
    }

    const PieceSet* moves = mTank->getMoves().toSet();
    const PieceSet* tiles = board->getPieceManager().getPieces();
    const PieceSet* deltas = mGame->getDeltaPieces();

    const PieceSet* shots = mGame->getCannonShot().getPath().toSet();
    if ( shots->empty() ) {
        shots = mShot->getPath().toSet();
    }

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
    renderOneRect( &rect, board, moves, tiles, deltas, shots, &painter );
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

void BoardWindow::keyPressEvent(QKeyEvent *ev)
{
    if ( !ev->isAutoRepeat()
      && mGame ) {
        switch( ev->key() ) {
        case Qt::Key_Space:
            mShot->fire( mTank->getRotation().toInt() );
            break;
        default:
            int rotation = keyToAngle(ev->key());
            if ( rotation >= 0 ) {
                mTank->move( rotation );
                mActiveMoveDirection = rotation;
            }
        }
    }
}

void BoardWindow::keyReleaseEvent(QKeyEvent *ev)
{
    if ( !ev->isAutoRepeat() ) {
        switch( ev->key() ) {
        case Qt::Key_Space:
            mShot->stop();
            break;

        case Qt::Key_Backspace:
            mTank->getMoves().eraseBack();
            break;

        default:
            if ( keyToAngle(ev->key()) >= 0 ) {
                mActiveMoveDirection = -1;
            }
        }
    }
}

void BoardWindow::mousePressEvent( QMouseEvent* event )
{
    switch( event->button() ) {
    case Qt::RightButton:
    {   QMenu menu;
        QAction* action;
        QString text( "level %1" );
        for( int level = 1; level <= BOARD_MAX_LEVEL; ++level ) {
            action = menu.addAction( text.arg(level) );
            action->setData( QVariant(level) );
        }
        action = menu.exec( event->globalPos() );
        if ( action ) {
            cout << "menu " << action->property("text").toString().toStdString() << " selected" << std::endl;
            mGame->getBoard()->load( action->data().toInt() );
        }
        break;
    }
    case Qt::LeftButton:
        mTank->stop();
        mGame->findPath( event->pos().x()/24, event->pos().y()/24, mTank->getX().toInt()/24, mTank->getY().toInt()/24, mTank->getRotation().toInt() );
        break;
    default:
        ;
    }
}

void BoardWindow::mouseReleaseEvent( QMouseEvent* event )
{
    switch( event->button() ) {
    case Qt::LeftButton:
        mTank->move();
        break;
    default:
        ;
    }
}

Tank* BoardWindow::getTank()
{
    return mTank;
}

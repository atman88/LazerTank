#include "BoardWindow.h"

BoardWindow::BoardWindow(QWindow *parent) : QWindow(parent)
{
    create();

    mBackingStore = new QBackingStore(this);
    mDirtyRegion = new QRegion();
    mStonePixmap.load(":/images/wall-stone.png");
    mDirtPixmap.load(":/images/dirt.png");
    mTilePixmap.load(":/images/tile-metal.png");
    mMoveIndicatorPixmap.load(":/images/move-indicator.png");

    mTank = new Tank(this);
    QObject::connect( mTank, &Tank::changed, this, &BoardWindow::onTankChanged );
    QObject::connect( mTank, &Tank::stopped, this, &BoardWindow::onTankStopped );

    mActiveMoveDirection = -1;
}

void BoardWindow::exposeEvent(QExposeEvent *)
{
    if ( mGame && isExposed() ) {
        Board* board = mGame->getBoard();
        if (board ) {
            QRect size( 0, 0, board->getWidth()*24, board->getHeight()*24 );
            *mDirtyRegion += size;
            renderNow();
        }
    }
}

void BoardWindow::onTankChanged( QRect rect )
{
    if ( mGame && !(rect.left() % 24) && !(rect.top() % 24) ) {
        mGame->onTankMoved( rect.left()/24, rect.top()/24 );
    }
    renderLater( &rect );
}

void BoardWindow::onTankStopped()
{
    int direction = mTank->getRotation().toInt();
    if ( direction == mActiveMoveDirection ) {
        mTank->move( direction );
    } else {
        mActiveMoveDirection = -1; // don't allow this to stick
    }
}

GameHandle BoardWindow::getGame() const
{
    return mGame->getHandle();
}

void BoardWindow::setGame(const GameHandle handle )
{
    // TODO disconnect any old game

    setProperty("GameHandle", QVariant::fromValue(handle));

    mGame = handle.game;
    if ( mGame ) {
        QObject::connect( mGame, &Game::pieceAdded, this, &BoardWindow::renderPieceLater );
        QObject::connect( mGame, &Game::pieceRemoved, this, &BoardWindow::erasePiece );
        QObject::connect( mGame, &Game::tankInitialized, mTank, &Tank::onUpdate );

        Board* board = mGame->getBoard();

        if ( board ) {
            QRect size( 0, 0, board->getWidth()*24, board->getHeight()*24 );
            setGeometry(size);
            *mDirtyRegion += size;
            mTank->onUpdate( mGame->getTankX(), mGame->getTankY() );
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
            *mDirtyRegion += size;
            renderNow();
        }
    }
}

void BoardWindow::renderNow()
{
    if (!isExposed())
        return;

    render( mDirtyRegion );
    mDirtyRegion->setRects(NULL, 0);
}

void BoardWindow::renderPieceLater( const Piece& piece )
{
    QRect dirty(piece.getX()*24, piece.getY()*24, 24, 24);
    renderLater( &dirty );
}

void BoardWindow::renderPiece( const Piece& piece, QPainter *painter )
{
    QPixmap* pixmap;
    switch( piece.getType() ) {
    case MOVE: pixmap = &mMoveIndicatorPixmap; break;
    case TILE: pixmap = &mTilePixmap;          break;

    default:
        return;
    }

    int x = piece.getX()*24;
    int y = piece.getY()*24;

    if ( piece.getAngle() ) {
        int centerX = x + 24/2;
        int centerY = y + 24/2;
        painter->translate(centerX, centerY);
        painter->rotate(piece.getAngle());
        painter->translate(-centerX, -centerY);
    }
    painter->drawPixmap( x, y, *pixmap);
    painter->resetTransform();
}

void BoardWindow::renderListAt( QPainter* painter, PieceSet::iterator* iterator, PieceSet::iterator end, Piece& pos )
{
    while( *iterator != end ) {
        int delta = (*iterator)->encodedPos() - pos.encodedPos();
        if ( delta > 0 ) {
            break;
        }
        if ( delta == 0 ) {
            // advance to the possible last duplicate
            Piece piece(  **iterator );
            while( *iterator != end && (++*iterator)->encodedPos() == pos.encodedPos() ) {
                piece = **iterator;
            }
            renderPiece( piece, painter );
            break;
        }
        ++*iterator;
    }
}

void BoardWindow::render(QRegion* region)
{
    if ( !mGame ) {
        return;
    }
    Board* board = mGame->getBoard();
    if ( board ) {
        mBackingStore->beginPaint(*region);
        QPaintDevice *device = mBackingStore->paintDevice();
        QPainter painter(device);

        QRect rect( region->boundingRect() );
        int minX = rect.left()/24;
        int minY = rect.top()/24;
        int maxX = rect.right()/24;
        int maxY = rect.bottom()/24;

        Piece pos(MOVE, minX, minY);
        QVariant v = mGame->property( "MoveList" );
        PieceList moveList = v.value<PieceList>();
        PieceSet moves;
        for( auto pieceListIterator = moveList.begin(); pieceListIterator != moveList.end(); ++pieceListIterator  ) {
            moves.insert( *pieceListIterator );
        }
        PieceSet::iterator moveIterator = moves.lower_bound( pos );

        v = board->property( "tiles" );
        PieceSet tiles = v.value<PieceSet>();
        PieceSet::iterator tileIterator = tiles.lower_bound( pos );

        for( int y = minY; y <= maxY; ++y ) {
            for( int x = minX; x <= maxX; ++x ) {
                switch( board->tileAt(x, y) ) {
                case STONE:
                    painter.drawPixmap( x*24, y*24, mStonePixmap);
                    break;
                case DIRT:
                    painter.drawPixmap( x*24, y*24, mDirtPixmap);
                    break;
                default:
                    painter.fillRect(x*24, y*24, 24, 24, Qt::blue);
                    break;
                }
                pos = Piece(MOVE, x, y);
                renderListAt( &painter, &moveIterator, moves.end(), pos );
                renderListAt( &painter, &tileIterator,  tiles.end(), pos );
            }
        }

        if ( region->intersects( *mTank->getRect() ) ) {
            mTank->paint( &painter );
        }

        mBackingStore->endPaint();
        mBackingStore->flush(*region);
    }
}

void BoardWindow::renderLater( QRect* rect )
{
    *mDirtyRegion += *rect;
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
        int rotation = keyToAngle(ev->key());
        if ( rotation >= 0 ) {
            mTank->move( rotation );
            mActiveMoveDirection = rotation;
        }
    }
}

void BoardWindow::erasePiece( const Piece& piece )
{
    QRect rect( piece.getX()*24, piece.getY()*24, 24, 24 );
    renderLater(&rect);
}

void BoardWindow::keyReleaseEvent(QKeyEvent *ev)
{
    if ( !ev->isAutoRepeat() && keyToAngle(ev->key()) >= 0 ) {
        mActiveMoveDirection = -1;
    }
}

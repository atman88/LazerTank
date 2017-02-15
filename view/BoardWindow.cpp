#include <iostream>
#include <QMenu>
#include <QAction>
#include "BoardWindow.h"

BoardWindow::BoardWindow(QWindow *parent) : QWindow(parent)
{
    create();

    mBackingStore = new QBackingStore(this);
    mDirtyRegion = new QRegion();
    mStonePixmap.load(":/images/wall-stone.png");
    mDirtPixmap.load(":/images/dirt.png");
    mTileSunkPixmap.load(":/images/tile-sunk.png");
    mFlagPixmap.load(":/images/flag.png");
    mTilePixmap.load(":/images/tile-metal.png");
    mMoveIndicatorPixmap.load(":/images/move-indicator.png");
    mShotStraightPixmap.load( ":/images/shot-straight.png");

    mTank = new Tank(this);
    QObject::connect( mTank, &Tank::changed,   this, &BoardWindow::renderLater      );
    QObject::connect( mTank, &Tank::pathAdded, this, &BoardWindow::renderPieceLater );

    mShot = new Shot(mTank);
    QObject::connect( mShot, &Shot::pathAdded,   this, &BoardWindow::renderPieceLater );
    QObject::connect( mShot, &Shot::pathRemoved, this, &BoardWindow::renderPieceLater );

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

void BoardWindow::onPieceStopped()
{
    mShot->stop();
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
        QObject::connect( mGame, &Game::pieceAdded,   this,  &BoardWindow::renderPieceLater );
        QObject::connect( mGame, &Game::pieceRemoved, this,  &BoardWindow::renderPieceLater );
        QObject::connect( mGame, &Game::pieceStopped, this,  &BoardWindow::onPieceStopped   );
        QObject::connect( mGame, &Game::pieceMoved,   this,  &BoardWindow::renderLater      );
        QObject::connect( mGame, &Game::rectDirty,    this,  &BoardWindow::renderLater      );
        QObject::connect( mTank, &Tank::moved,        mGame, &Game::onTankMoved             );

        Board* board = mGame->getBoard();
        if ( board ) {
            onBoardLoaded();
            QObject::connect( board, &Board::boardLoaded, this, &BoardWindow::onBoardLoaded );
        }
    }
}

void BoardWindow::onBoardLoaded()
{
    Board* board = mGame->getBoard();
    if ( board ) {
        QRect size( 0, 0, board->getWidth()*24, board->getHeight()*24 );
        setGeometry(size);
        *mDirtyRegion += size;
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
    renderLater( dirty );
}

void BoardWindow::renderPiece( PieceType type, int x, int y, int angle, QPainter *painter )
{
    QPixmap* pixmap;
    switch( type ) {
    case MOVE:          pixmap = &mMoveIndicatorPixmap; break;
    case TILE:          pixmap = &mTilePixmap;          break;
    case SHOT_STRAIGHT: pixmap = &mShotStraightPixmap;  break;

    default:
        return;
    }

    if ( angle ) {
        int centerX = x + 24/2;
        int centerY = y + 24/2;
        painter->translate(centerX, centerY);
        painter->rotate(angle);
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
            renderPiece( piece.getType(), piece.getX()*24, piece.getY()*24, piece.getAngle(), painter );
            break;
        }
        ++*iterator;
    }
}

void loadDrawSet( PieceList& list, PieceSet& set )
{
    for( auto iterator = list.begin(); iterator != list.end(); ++iterator  ) {
        set.insert( *iterator );
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
        PieceSet moves;
        loadDrawSet( mTank->getMoves(), moves );
        PieceSet::iterator moveIterator = moves.lower_bound( pos );

        QVariant v = board->property( "tiles" );
        PieceSet tiles = v.value<PieceSet>();
        PieceSet::iterator tileIterator = tiles.lower_bound( pos );

        v = mShot->property( "path" );
        PieceList shotList = v.value<PieceList>();
        PieceSet shots;
        loadDrawSet( shotList, shots );
        PieceSet::iterator shotIterator = shots.lower_bound( pos );

        for( int y = minY; y <= maxY; ++y ) {
            for( int x = minX; x <= maxX; ++x ) {
                switch( board->tileAt(x, y) ) {
                case STONE:
                    painter.drawPixmap( x*24, y*24, mStonePixmap);
                    break;
                case DIRT:
                    painter.drawPixmap( x*24, y*24, mDirtPixmap);
                    break;
                case TILE_SUNK:
                    painter.drawPixmap( x*24, y*24, mTileSunkPixmap);
                    break;
                case FLAG:
                    painter.drawPixmap( x*24, y*24, mFlagPixmap);
                    break;
                default:
                    painter.fillRect(x*24, y*24, 24, 24, Qt::blue);
                    break;
                }
                pos = Piece(MOVE, x, y);
                renderListAt( &painter, &tileIterator, tiles.end(), pos );
                renderListAt( &painter, &moveIterator, moves.end(), pos );
                renderListAt( &painter, &shotIterator, shots.end(), pos );
            }
        }

        PieceType movingPieceType = mGame->getMovingPieceType();
        if ( movingPieceType != NONE ) {
            renderPiece( movingPieceType, mGame->getPieceX().toInt(), mGame->getPieceY().toInt(), 0, &painter );
        }
        if ( region->intersects( mTank->getRect() ) ) {
            mTank->paint( &painter );
        }

        mBackingStore->endPaint();
        mBackingStore->flush(*region);
    }
}

void BoardWindow::renderLater( const QRect& rect )
{
    *mDirtyRegion += rect;
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
            if ( !mTank->isMoving() ) {
                mShot->fire( mTank->getRotation().toInt(), mTank->getX().toInt()/24, mTank->getY().toInt()/24 );
            }
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

        default:
            if ( keyToAngle(ev->key()) >= 0 ) {
                mActiveMoveDirection = -1;
            }
        }
    }
}

void BoardWindow::mousePressEvent( QMouseEvent* event )
{
    if ( event->button() == Qt::RightButton ) {
        QMenu menu;
        QAction* action;
        QString text( "level%1" );
        for( int level = 1; level <= 3; ++level ) {
            action = menu.addAction( text.arg(level) );
            action->setData( QVariant(level) );
        }
        action = menu.exec( event->globalPos() );
        if ( action ) {
            cout << "menu " << action->property("text").toString().toStdString() << " selected" << std::endl;
            mGame->getBoard()->load( action->data().toInt() );
        }
    }
}

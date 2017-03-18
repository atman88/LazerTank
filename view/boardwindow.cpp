#include <iostream>
#include <QMenu>
#include <QAction>
#include <QMessageBox>

#include "boardwindow.h"
#include "util/renderutils.h"
#include "util/imageutils.h"
#include "controller/game.h"

using namespace std;

BoardWindow::BoardWindow(QWindow *parent) : QWindow(parent), mFocus(MOVE), mGame(0)
{
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
    setFlags( Qt::Dialog );
    create();

    mCaptureAction = std::make_shared<PathSearchAction>(this);
    mPathToAction  = std::make_shared<PathSearchAction>(this);
    mBackingStore = new QBackingStore(this);

    if ( game ) {
        mGame = game;
        setProperty("GameHandle", game->property("GameHandle"));

        onBoardLoaded();
        Board* board = game->getBoard();
        QObject::connect( board, &Board::boardLoaded,   this, &BoardWindow::onBoardLoaded     );
        QObject::connect( board, &Board::tileChangedAt, this, &BoardWindow::renderSquareLater );

        PieceSetManager* pm = board->getPieceManager();
        QObject::connect( pm, &PieceSetManager::erasedAt,   this, &BoardWindow::renderSquareLater );
        QObject::connect( pm, &PieceSetManager::insertedAt, this, &BoardWindow::renderSquareLater );

        QObject::connect( &mSpeedAction, &QAction::toggled, mGame->getSpeedController(), &SpeedController::setHighSpeed );
        QObject::connect( &mUndoMoveAction,  &QAction::triggered, mGame, &Game::undoLastMove );
        QObject::connect( &mClearMovesAction,&QAction::triggered, mGame->getTank(), &Tank::clearMoves );
    }
}

void BoardWindow::onBoardLoaded()
{
    if ( mGame ) {
        Board* board = mGame->getBoard();
        QRect size( 0, 0, board->getWidth()*24, board->getHeight()*24 );

        QRect myGeometry = geometry();
        myGeometry.setSize( size.size() );

        // center it roughly if it's position isn't initialized, :
        if ( !isExposed() && !myGeometry.x() && !myGeometry.y() ) {
            if ( QScreen* myScreen = screen() ) {
                QRect available = myScreen->availableGeometry();
                myGeometry.moveTo( (available.width() -myGeometry.width() )/2,
                                   (available.height()-myGeometry.height())/2  );
            }
        }
        setGeometry(myGeometry);

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
    while( iterator != end ) {
        if ( !(*iterator)->render( dirty, painter ) ) {
            break;
        }
        ++iterator;
    }
}

QMenu& BoardWindow::getMenu()
{
    return mMenu;
}

void BoardWindow::render( const QRect* rect, QPainter* painter )
{
    Board* board = mGame->getBoard();
    const PieceMultiSet* moves = mGame->getTank()->getMoves()->toMultiSet();
    const PieceSet* tiles = board->getPieceManager()->getPieces();
    const PieceSet* deltas = mGame->getDeltaPieces();

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

    if ( mFocus != TANK ) {
        // render the moves beneath (i.e. before) the tank and it's pushes when not focused on
        // the tank:
        renderListIn( moveIterator, moves->end(), rect, painter );
    }

    if ( mGame ) {
        mGame->getTankPush().render( rect, painter );
        mGame->getShotPush().render( rect, painter );
        mGame->getTank()->render( rect, painter );
        if ( mFocus == TANK ) {
            // render the moves ontop of (i.e. after) the tank and it's pushes when focus is at
            // the tank:
            renderListIn( moveIterator, moves->end(), rect, painter );
        }
        mGame->getCannonShot().render( painter );
    }
}

void BoardWindow::render(QRegion* region)
{
    mBackingStore->beginPaint(*region);
    QPaintDevice *device = mBackingStore->paintDevice();
    QPainter painter(device);
    QRect rect = region->boundingRect();

    render( &rect, &painter );

    mBackingStore->endPaint();
    mBackingStore->flush(*region);
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

void BoardWindow::showMenu( QPoint* globalPos, int col, int row )
{
    if ( mGame ) {
        QPoint pos;

        //
        // create the menu on first use
        //
        if ( mMenu.isEmpty() ) {
            QKeySequence shootSeq( Qt::Key_Space );
            QKeySequence undoSeq( Qt::Key_Backspace );
            QKeySequence clearSeq( Qt::CTRL|Qt::Key_Backspace );
            QKeySequence speedSeq( Qt::Key_S );
            QKeySequence captureSeq( Qt::Key_C );

            mMenu.addAction( "shoot& ", mGame->getTank(), &Tank::fire, shootSeq );
            mUndoMoveAction.setText("&Undo");
            mClearMovesAction.setText("Clear Moves");
            mSpeedAction.setText( "&Speed Boost" );
            mPathToAction->setText( "Move &Here" );
            mCaptureAction->setText( "&Capture Flag" );
            mReloadAction.setText( "&Restart Level" );

            mSpeedAction.setShortcut( speedSeq );
            mSpeedAction.setCheckable(true);
            mCaptureAction->setShortcut( captureSeq );
            mUndoMoveAction.setShortcut( undoSeq );
            mClearMovesAction.setShortcut(clearSeq);

            mMenu.addAction( &mSpeedAction      );
            mMenu.addAction( &mUndoMoveAction   );
            mMenu.addAction( &mClearMovesAction );
            mMenu.addAction( &(*mPathToAction)  );
            mMenu.addAction( &(*mCaptureAction) );
            mMenu.addAction( &mReloadAction     );

            QAction* action = mMenu.addAction( QString("Select &Level..") );
            action->setMenu( &mLevelsMenu );
            QString text( "level %1" );
            for( int level = 1; level <= BOARD_MAX_LEVEL; ++level ) {
                action = mLevelsMenu.addAction( text.arg(level) );
                action->setData( QVariant(level) );
            }

            mMenu.addAction( "E&xit", this, &QWindow::close );
        }

        //
        // freshen dynamic menu item properties
        //
        Board* board = mGame->getBoard();
        mSpeedAction.setChecked( mGame->getSpeedController()->getHighSpeed() );
        mReloadAction.setData( QVariant( board->getLevel()) );

        std::shared_ptr<PathSearchAction> actions[2] = { mCaptureAction, mPathToAction };
        mCaptureAction->setCriteria( mFocus, board->getFlagCol(), board->getFlagRow(), true );
        int nActions;
        if ( col < 0 ) {
            nActions = 1;
            mPathToAction->setVisible( false );
        } else {
            nActions = 2;
            mPathToAction->setVisible( true );
            mPathToAction->setCriteria( mFocus, col, row, true );
        }
        mGame->getPathFinderController()->testActions( actions, nActions );

        bool movesPending = mGame->getTank()->getMoves()->size() > 0;
        mUndoMoveAction.setEnabled( movesPending );
        mClearMovesAction.setEnabled( movesPending );

        //
        // launch menu
        //
        if ( globalPos ) {
            pos = *globalPos;
        } else {
            // choose a reasonable position:
            pos.setX( geometry().right() );
            pos.setY( geometry().top()   );
        }
        QAction* action = mMenu.exec( pos );
        if ( action == &(*mCaptureAction) ) {
            mGame->getPathFinderController()->doAction( mCaptureAction );
        } else if ( action == &(*mPathToAction) ) {
            mGame->getPathFinderController()->doAction( mPathToAction );
        } else if ( action ) {
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
            if ( !ev->modifiers() ) {
                showMenu();
            }
            break;

        case Qt::Key_Control:
            moveFocus( TANK );
            break;

        case Qt::Key_Space:
            mGame->getTank()->fire();
            break;

        case Qt::Key_C: // attempt to capture the flag
        {   Board* board = mGame->getBoard();
            mCaptureAction->setCriteria( mFocus, board->getFlagCol(), board->getFlagRow(), false );
            mGame->getPathFinderController()->doAction( mCaptureAction );
            break;
        }
        default:
            if ( !ev->modifiers() ) {
                int rotation = keyToAngle(ev->key());
                if ( rotation >= 0 ) {
                    mGame->getTank()->move( rotation );
                }
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

        case Qt::Key_Control:
            moveFocus( MOVE );
            break;

        case Qt::Key_S:
            mSpeedAction.toggle();
            break;

        case Qt::Key_C:
                mGame->getTank()->wakeup();
                break;

        case Qt::Key_Backspace:
            if ( ev->modifiers() == Qt::ControlModifier ) {
                mGame->getTank()->clearMoves();
            } else {
                mGame->undoLastMove();
            }
            break;
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
        showMenu( &globalPos, event->pos().x()/24, event->pos().y()/24 );
        break;
    }
    case Qt::LeftButton:
        mPathToAction->setCriteria( mFocus, event->pos().x()/24, event->pos().y()/24, false );
        mGame->getPathFinderController()->doAction( mPathToAction );
        break;
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
        mGame->getTank()->wakeup();
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

void BoardWindow::moveFocus( PieceType what )
{
    mFocus = what;
    emit focusChanged( what );
}

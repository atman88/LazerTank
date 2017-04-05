#include <iostream>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QTextBrowser>

#include "boardwindow.h"
#include "util/renderutils.h"
#include "util/imageutils.h"
#include "controller/game.h"

using namespace std;

BoardWindow::BoardWindow(QWindow *parent) : QWindow(parent), mFocus(MOVE), mGame(0), mHelpWidget(0)
#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
  , mUpdatePending(false)
#endif
{
}

BoardWindow::~BoardWindow()
{
    if ( mHelpWidget ) {
        delete mHelpWidget;
    }
    delete mBackingStore;
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

        QObject::connect( &TO_QACTION(mSpeedAction), &QAction::toggled, mGame->getSpeedController(), &SpeedController::setHighSpeed );
        QObject::connect( &TO_QACTION(mUndoMoveAction),  &QAction::triggered, mGame, &Game::undoLastMove );
        QObject::connect( &TO_QACTION(mClearMovesAction),&QAction::triggered, mGame->getMoveController(), &MoveController::clearMoves );

        QObject::connect( game->getMoveController(), &MoveController::replayFinished, this, &BoardWindow::onReplayFinished );
    }
}

void BoardWindow::onBoardLoaded()
{
    if ( mGame ) {
        Board* board = mGame->getBoard();
        QRect size( 0, 0, board->getWidth()*24, board->getHeight()*24 );

        QRect myGeometry = geometry();
        myGeometry.setSize( size.size() );

        // center it roughly if it's position isn't initialized
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

void BoardWindow::showHelp()
{
    if ( !mHelpWidget ) {
        mHelpWidget = new QTextBrowser();
        mHelpWidget->setWindowTitle( QString("LazerTank Help") );

        if ( QScreen* myScreen = screen() ) {
            QRect rect = myScreen->availableGeometry();
            rect.setWidth( rect.width()/2 );
            rect.setHeight( (rect.height() * 2) / 3 );
            mHelpWidget->setGeometry( rect );
        }
        mHelpWidget->setSource( QUrl::fromLocalFile(":/help/qlthelp.html") );
    }
    mHelpWidget->show();
}

void BoardWindow::onReplayFinished()
{
    if ( !mReplayTextRenderRect.isNull() ) {
        renderLater( mReplayTextRenderRect );
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
#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
    mUpdatePending = false;
#endif
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
    if ( !mGame ) {
        return;
    }
    Board* board = mGame->getBoard();
    MoveController* moveController = mGame->getMoveController();
    const PieceMultiSet* moves = moveController->getMoves()->toMultiSet();
    const PieceSet* tiles = board->getPieceManager()->getPieces();
    const PieceSet* deltas = mGame->getDeltaPieces();

    int minX = rect->left()/24;
    int minY = rect->top() /24;
    int maxX = (rect->right() +24-1)/24;
    int maxY = (rect->bottom()+24-1)/24;

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

    bool usingFutureShotPen = false;
    for( auto it : *moveController->getFutureShots()->getPaths() ) {
        if ( rect->intersects( it.getBounds() ) ) {
            if ( !usingFutureShotPen ) {
                QPen pen( mGame->getTank()->getShot().getPen() );
                pen.setStyle( Qt::DashLine );
                painter->setPen( pen );
                usingFutureShotPen = true;
            }
            painter->drawPath( *it.toQPath() );
        }
    }

    if ( mFocus != TANK ) {
        // render the moves beneath (i.e. before) the tank and it's pushes when not focused on
        // the tank:
        renderListIn( moveIterator, moves->end(), rect, painter );
    }

    mGame->getTankPush().render( rect, painter );
    mGame->getShotPush().render( rect, painter );
    mGame->getTank()->render( rect, painter );
    if ( mFocus == TANK ) {
        // render the moves ontop of (i.e. after) the tank and it's pushes when focus is at
        // the tank:
        renderListIn( moveIterator, moves->end(), rect, painter );
    }
    mGame->getCannonShot().render( painter );

    if ( mGame->getMoveController()->replaying() ) {
        QFont font = painter->font();
        font.setBold(true);
        font.setItalic(true);
        font.setPixelSize(36);
        painter->setFont( font );

        if ( mReplayTextRenderRect.isNull() ) {
            mReplayTextRenderRect = painter->boundingRect( QRect(0,0,width(),height()), Qt::AlignCenter, QString("REPLAY") );
        }

        if ( rect->intersects( mReplayTextRenderRect ) ) {
            QPen pen;
            pen.setColor( QColor(32,32,255,127) );
            painter->setPen( pen );
            painter->drawText( mReplayTextRenderRect, QString("REPLAY") );
        }
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

    if ( event->type() == QEvent::Resize ) {
        // nullify this given it is stale:
        mReplayTextRenderRect = QRect();
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

void BoardWindow::showMenu( QPoint* globalPos, ModelPoint p )
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

            mMenu.addAction( "shoot& ", mGame->getMoveController(), SLOT(fire()), shootSeq );
            TO_QACTION(mUndoMoveAction).setText("&Undo");
            TO_QACTION(mClearMovesAction).setText("Clear Moves");
            TO_QACTION(mSpeedAction).setText( "&Speed Boost" );
            mPathToAction->setText( "Move &Here" );
            mCaptureAction->setText( "&Capture Flag" );
            TO_QACTION(mReloadAction).setText( "&Restart Level" );

            TO_QACTION(mSpeedAction).setShortcut( speedSeq );
            TO_QACTION(mSpeedAction).setCheckable(true);
            mCaptureAction->setShortcut( captureSeq );
            TO_QACTION(mUndoMoveAction).setShortcut( undoSeq );
            TO_QACTION(mClearMovesAction).setShortcut(clearSeq);

            mMenu.addAction( &TO_QACTION(mSpeedAction)      );
            mMenu.addAction( &TO_QACTION(mUndoMoveAction)   );
            mMenu.addAction( &TO_QACTION(mClearMovesAction) );
            mMenu.addAction( &(*mPathToAction)  );
            mMenu.addAction( &(*mCaptureAction) );
            mMenu.addAction( &TO_QACTION(mReloadAction)     );

            QAction* action = mMenu.addAction( QString("Select &Level..") );
            action->setMenu( &mLevelsMenu );
            QString text( "level %1" );
            for( int level = 1; level <= BOARD_MAX_LEVEL; ++level ) {
                action = mLevelsMenu.addAction( text.arg(level) );
                action->setData( QVariant(level) );
            }

            mMenu.addAction( QString("Replay"), mGame, SLOT(replayLevel()) );
            mMenu.addAction( QString("&Help"), this, SLOT(showHelp()) );
            mMenu.addAction( QString("E&xit"), this, SLOT(close()) );
        }

        //
        // freshen dynamic menu item properties
        //
        Board* board = mGame->getBoard();
        TO_QACTION(mSpeedAction).setChecked( mGame->getSpeedController()->getHighSpeed() );
        TO_QACTION(mReloadAction).setData( QVariant( board->getLevel()) );

        std::shared_ptr<PathSearchAction> actions[2] = { mCaptureAction, mPathToAction };
        mCaptureAction->setCriteria( mFocus, board->getFlagPoint(), true );
        int nActions;
        if ( p.mCol < 0 ) {
            nActions = 1;
            mPathToAction->setVisible( false );
        } else {
            nActions = 2;
            mPathToAction->setVisible( true );
            mPathToAction->setCriteria( mFocus, p, true );
        }
        mGame->getPathFinderController()->testActions( actions, nActions );

        bool movesPending = mGame->getMoveController()->getMoves()->size() > 0;
        TO_QACTION(mUndoMoveAction).setEnabled( movesPending );
        TO_QACTION(mClearMovesAction).setEnabled( movesPending );

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
            } else {
                action->trigger();
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
            mGame->getMoveController()->fire();
            break;

        case Qt::Key_C: // attempt to capture the flag
        {   Board* board = mGame->getBoard();
            mCaptureAction->setCriteria( mFocus, board->getFlagPoint(), false );
            mGame->getPathFinderController()->doAction( mCaptureAction );
            break;
        }
        default:
            if ( !ev->modifiers() ) {
                int rotation = keyToAngle(ev->key());
                if ( rotation >= 0 ) {
                    mGame->getMoveController()->move( rotation );
                } else if ( ev->key() >= Qt::Key_0 && ev->key() <= Qt::Key_9 ) {
                    mGame->getMoveController()->fire( ev->key() - Qt::Key_0 );
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
            TO_QACTION(mSpeedAction).toggle();
            break;

        case Qt::Key_C:
                mGame->getMoveController()->wakeup();
                break;

        case Qt::Key_Backspace:
            if ( ev->modifiers() == Qt::ControlModifier ) {
                mGame->getMoveController()->clearMoves();
            } else {
                mGame->undoLastMove();
            }
            break;

        case Qt::Key_F1:
            showHelp();
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
        showMenu( &globalPos, ModelPoint( event->pos() ) );
        break;
    }
    case Qt::LeftButton:
        mPathToAction->setCriteria( mFocus, ModelPoint( event->pos() ), false );
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
        mGame->getMoveController()->wakeup();
        break;
    default:
        ;
    }
}

void BoardWindow::moveFocus( PieceType what )
{
    mFocus = what;
    emit focusChanged( what );
}

#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
void BoardWindow::requestUpdate()
{
    if ( !mUpdatePending ) {
        QTimer::singleShot( 17, this, &BoardWindow::renderNow );
        mUpdatePending = true;
    }
}
#endif

#include <QMenu>
#include <QApplication>
#include <QAction>
#include <QMessageBox>
#include <QTextBrowser>
#include <QWhatsThis>

#include "boardwindow.h"
#include "boardrenderer.h"
#include "replaytext.h"
#include "controller/gameregistry.h"
#include "controller/game.h"
#include "controller/speedcontroller.h"
#include "controller/movecontroller.h"
#include "controller/pathfindercontroller.h"
#include "model/tank.h"
#include "model/push.h"
#include "model/shotmodel.h"
#include "model/level.h"
#include "view/levelchooser.h"
#include "util/recorder.h"
#include "util/imageutils.h"
#include "util/helputils.h"

#define TILE_SIZE 24
#define STATUS_HEIGHT 18

BoardWindow::BoardWindow(QWindow *parent) : QWindow(parent), mBackingStore(0), mRenderer(TILE_SIZE), mRenderedOnce(false),
  mGameInitialized(false), mHelpWidget(0), mReplayText(0), mForbiddenCursor(0), mStatusDirty(false)
#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
  , mUpdatePending(false)
#endif
{
    setFlags( Qt::Dialog );
    create();
    // default our size to something reasonable; choosing 6x6 because that is the size of level 1
    setSize( 6, 6 );
}

BoardWindow::~BoardWindow()
{
    if ( mHelpWidget ) {
        delete mHelpWidget;
    }
    if ( mReplayText ) {
        delete mReplayText;
    }
    if ( mForbiddenCursor ) {
        delete mForbiddenCursor;
    }
    delete mBackingStore;
}

void BoardWindow::setSize( int cols, int rows )
{
    QRect size( 0, 0, cols * TILE_SIZE, rows * TILE_SIZE + STATUS_HEIGHT );
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
    mDirtyRegion = size;
    renderNow();
}

void BoardWindow::init( GameRegistry* registry )
{
    Game& game = registry->getGame();
    MoveController& moveController = registry->getMoveController();

    Board* board = game.getBoard();
    QObject::connect( board, &Board::tileChangedAt, this, &BoardWindow::renderSquareLater, Qt::DirectConnection );

    PieceSetManager& pm = board->getPieceManager();
    QObject::connect( &pm, &PieceSetManager::erasedAt,   this, &BoardWindow::renderSquareLater, Qt::DirectConnection );
    QObject::connect( &pm, &PieceSetManager::insertedAt, this, &BoardWindow::renderSquareLater, Qt::DirectConnection );

    TO_QACTION(mSpeedAction).setCheckable(true);

    QObject::connect( &TO_QACTION(mSpeedAction), &QAction::toggled, &registry->getSpeedController(), &SpeedController::setHighSpeed );
    QObject::connect( &TO_QACTION(mUndoMoveAction),  &QAction::triggered, &moveController, &MoveController::undoLastMove );
    QObject::connect( &TO_QACTION(mClearMovesAction),&QAction::triggered, &moveController, &MoveController::undoMoves );
    QObject::connect( &TO_QACTION(mReplayAction),    &QAction::triggered, &game, &Game::replayLevel );

    QObject::connect( &moveController, &MoveController::dragStateChanged, this, &BoardWindow::setCursorDragState );
    QObject::connect( &moveController, &MoveController::tileDragFocusChanged, &mDragMarker, &TileDragMarker::setFocus );
    QObject::connect( &mDragMarker, &TileDragMarker::rectDirty, this, &BoardWindow::renderLater, Qt::DirectConnection );

    QObject::connect( &game, &Game::boardLoaded, this, &BoardWindow::onBoardLoaded, Qt::DirectConnection );

    QObject::connect( &registry->getLevelList(), &LevelList::levelUpdated, this, &BoardWindow::onLevelUpdated );

    QObject::connect( &registry->getRecorder(), &Recorder::recordedCountChanged, this, &BoardWindow::onRecordedCountChanged );
}

void BoardWindow::onBoardLoaded()
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        Board* board = registry->getGame().getBoard();
        setSize( board->getWidth(), board->getHeight() );
        int level = board->getLevel();
        if ( level > 0 ) {
            QString title( QString("Level %1").arg(level) );
            setTitle( title );
        }
    }
}

void BoardWindow::onLevelUpdated( const QModelIndex& index )
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        if ( const Level* level = registry->getLevelList().at( index.row() ) ) {
            Board* board = registry->getGame().getBoard();
            if ( level->getNumber() == board->getLevel() ) {
                renderLater( QRect(0,board->getHeight()*TILE_SIZE,geometry().width(),STATUS_HEIGHT) );
            }
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
            rect.moveTo(50,50); // avoid having the frame clipped offscreen on MSWindows
            mHelpWidget->setGeometry( rect );
        }
        mHelpWidget->setSource( QUrl::fromLocalFile(":/help/qlthelp.html") );
    }
    mHelpWidget->show();
}

void BoardWindow::chooseLevel()
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        LevelChooser* chooser = new LevelChooser( registry->getLevelList(), registry->getBoardPool() );
        chooser->setAttribute( Qt::WA_DeleteOnClose );
        QObject::connect( chooser, &LevelChooser::levelChosen, this, &BoardWindow::loadLevel );

        if ( QScreen* myScreen = screen() ) {
            QSize s = chooser->preferredSize();
            chooser->setGeometry( 0, 0, s.width(), std::min( myScreen->availableGeometry().height(), s.height() ) );
        }
        chooser->move( frameGeometry().right(), chooser->frameGeometry().top() );

        chooser->setVisible(true);
        chooser->setSelectedLevel( registry->getGame().getBoard()->getLevel() );
        QEventLoop eventLoop;
        eventLoop.exec( QEventLoop::DialogExec );
    }
}

void BoardWindow::loadLevel( int number )
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        registry->getGame().loadMasterBoard( number );
    }
}

void BoardWindow::renderSquareLater( ModelPoint point )
{
    QRect dirty(point.mCol*TILE_SIZE, point.mRow*TILE_SIZE, TILE_SIZE, TILE_SIZE);
    renderLater( dirty );
}

QMenu& BoardWindow::getMenu()
{
    return mMenu;
}

bool BoardWindow::isPaintable() const
{
    return mRenderedOnce;
}

void BoardWindow::renderBoard( const QRect* rect, GameRegistry* registry, QPainter* painter )
{
//std::cout << "render " << rect->x() << "," << rect->y() << " " << rect->width() << "x" << rect->height() << std::endl;
    Board* board = registry->getGame().getBoard();
    Tank& tank = registry->getTank();
    MoveController& moveController = registry->getMoveController();

    mRenderer.render( rect, board, painter );

    bool tankIsProminent = moveController.getFocus() != TANK && moveController.getDragState() == Inactive;
    if ( tankIsProminent && moveController.getDragState() != Inactive ) {
        if ( Piece* piece = moveController.getMoves().getBack() ) {
            tankIsProminent = !tank.getPoint().equals( *piece );
        }
    }

    if ( tankIsProminent ) {
        // render the moves beneath (i.e. before) the tank:
        mRenderer.renderMoves( rect, registry, painter );
    }

    registry->getTankPush().render( rect, mRenderer, painter );
    registry->getShotPush().render( rect, mRenderer, painter );
    tank.render( rect, painter );

    if ( !tankIsProminent ) {
        // render the moves on top of (i.e. after) the tank:
        mRenderer.renderMoves( rect, registry, painter );
    }

    mDragMarker.render( rect, painter );
    registry->getCannonShot().render( painter );

    if ( moveController.replaying() ) {
        if ( !mReplayText ) {
            mReplayText = new ReplayText( this, QString("REPLAY") );
            QObject::connect( mReplayText, &ReplayText::dirty, this, &BoardWindow::renderLater, Qt::DirectConnection );
        }

        mReplayText->render( rect, painter );
    }
}

void BoardWindow::renderStatus( QRect& statusRect, GameRegistry* registry, QPainter* painter )
{
    QFont font = painter->font();
    font.setPixelSize( STATUS_HEIGHT-4 );
    painter->setFont(font);
    painter->setPen( Qt::gray );

    QRect r( statusRect );
    r.setLeft( r.left()+2 );
    painter->drawText( r, Qt::AlignVCenter|Qt::AlignLeft, QString::number(registry->getRecorder().getRecordedCount()) );

    if ( const Level* level = registry->getLevelList().find( registry->getGame().getBoard()->getLevel() ) ) {
        if ( int completedCount = level->getCompletedCount() ) {
            QString s = QString::number(completedCount);
            r = painter->boundingRect( statusRect, Qt::AlignVCenter, s );
            r.moveLeft( statusRect.width()-r.width()-2 );
            painter->drawText( r, 0, s );
            const QPixmap* checkmark = ResourcePixmap::getPixmap(COMPLETE_CHECKMARK);
            painter->drawPixmap( r.left()-checkmark->width()-2, statusRect.bottom()-checkmark->height(), *checkmark );
        }
    }
    mStatusDirty = false;
}

void BoardWindow::renderNow()
{
    if ( mDirtyRegion.isNull() && mRenderedOnce ) {
        return;
    }

    if ( mBackingStore && isExposed() ) {
        if ( QPaintDevice *device = mBackingStore->paintDevice() ) {
            GameRegistry* registry = getRegistry(this);
            bool boardLoaded = registry ? registry->getGame().isBoardLoaded() : false;
            QRect rect;
            if ( boardLoaded ) {
                mDirtyRegion.swap( mRenderRegion );
                mDirtyRegion.setRects(0,0);
                rect = mRenderRegion.boundingRect();
            } else {
                rect = QRect( QPoint(0,0), size() );
                mRenderRegion = rect;
            }

            QRect statusRect;
            bool statusFilled = false;
            if ( boardLoaded ) {
                int boardHeight = registry->getGame().getBoard()->getHeight() * TILE_SIZE;
                statusFilled = rect.bottom() > boardHeight;
                if ( statusFilled || mStatusDirty ) {
                    statusRect = QRect( 0, boardHeight, geometry().width(), STATUS_HEIGHT );
                }
                if ( !statusFilled && mStatusDirty ) {
                    mRenderRegion += statusRect;
                }
            }

            mBackingStore->beginPaint( mRenderRegion );
            QPainter painter( device );

            painter.fillRect( rect, Qt::black );

            if ( boardLoaded ) {
                if ( !statusFilled && mStatusDirty ) {
                    painter.fillRect( statusRect, Qt::black );
                }
                renderStatus( statusRect, registry, &painter );

                renderBoard( &rect, registry, &painter );
            }

            mBackingStore->endPaint();
            mBackingStore->flush( mRenderRegion );

#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
            mUpdatePending = false;
#endif
            if ( !mRenderedOnce ) {
                mRenderedOnce = true;
                emit paintable();
            }
        }
    }
}

void BoardWindow::renderLater( const QRect& rect )
{
    mDirtyRegion += rect;
    requestUpdate();
}

bool BoardWindow::resizeInternal( const QSize& size )
{
    mDirtyRegion = QRect( QPoint(0,0), size );

    if ( mBackingStore && mBackingStore->size() != size ) {
        mBackingStore->resize( size );
        if ( mReplayText ) {
            mReplayText->onResize();
        }
        return true;
    }
    return false;
}

void BoardWindow::showEvent(QShowEvent*)
{
    if ( !mBackingStore ) {
        mBackingStore = new QBackingStore(this);

        // sizing it on first here because I don't see the backing store constructor initializing it's size
        resizeInternal( size() );
    }
    renderNow();
}

void BoardWindow::resizeEvent( QResizeEvent* resizeEvent )
{
    if ( resizeInternal( resizeEvent->size() ) ) {
        renderNow();
    }
}

void BoardWindow::exposeEvent( QExposeEvent* )
{
    if ( mBackingStore ) {
        mDirtyRegion = QRect( QPoint(0,0), mBackingStore->size() );
        renderNow();
    }
}

bool BoardWindow::event( QEvent* event )
{
    QEvent::Type type = event->type();
    if ( type == QEvent::UpdateRequest ) {
        if ( isExposed() ) {
            renderNow();
        }
        return true;
    }

    if ( type == QltWhatsThisEvent::getEventType() ) {
        QltWhatsThisEvent* whatsThisEvent = static_cast<QltWhatsThisEvent*>( event );
        QWhatsThis::showText( whatsThisEvent->getPos(), whatsThisEvent->getHelpText() );
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
    if ( GameRegistry* registry = getRegistry(this) ) {
        QPoint pos;

        PathSearchAction& captureAction = registry->getCaptureAction();
        PathSearchAction& pathToAction  = registry->getPathToAction();

        //
        // create the menu on first use
        //
        if ( mMenu.isEmpty() ) {
            mMenu.addAction( "shoot& ", &registry->getMoveController(), SLOT(fire()), Qt::Key_Space );
            TO_QACTION(mWhatsThisAction).setText( "What's this?" );
            TO_QACTION(mUndoMoveAction).setText( "&Undo" );
            TO_QACTION(mClearMovesAction).setText( "Clear Moves" );
            TO_QACTION(mSpeedAction).setText( "&Speed Boost" );
            pathToAction.setText( "Move &Here" );
            captureAction.setText( "&Capture Flag" );
            TO_QACTION(mReloadAction).setText( "&Restart Level" );
            TO_QACTION(mReplayAction).setText( "&Auto Replay" );

            TO_QACTION(mSpeedAction).setShortcut( Qt::Key_S );
            captureAction.setShortcut( Qt::Key_C );
            TO_QACTION(mUndoMoveAction).setShortcut( Qt::Key_Backspace );
            TO_QACTION(mClearMovesAction).setShortcut( Qt::CTRL|Qt::Key_Backspace );
            TO_QACTION(mReloadAction).setShortcut( Qt::ALT|Qt::Key_R );
            TO_QACTION(mReplayAction).setShortcut( Qt::ALT|Qt::Key_A );

            mMenu.addAction( &TO_QACTION(mWhatsThisAction)  );
            mMenu.addAction( &TO_QACTION(mSpeedAction)      );
            mMenu.addAction( &TO_QACTION(mUndoMoveAction)   );
            mMenu.addAction( &TO_QACTION(mClearMovesAction) );
            mMenu.addAction( &pathToAction );
            mMenu.addAction( &captureAction );
            mMenu.addAction( &TO_QACTION(mReloadAction)     );
            mMenu.addAction( "Select &Level..", this, SLOT(chooseLevel()), Qt::ALT|Qt::Key_L );
            mMenu.addAction( &TO_QACTION(mReplayAction) );
            mMenu.addAction( "&Help", this, SLOT(showHelp()) );
            mMenu.addAction( "About Qt", qApp, &QApplication::aboutQt );
            mMenu.addAction( "E&xit", this, SLOT(close()) );
        }

        //
        // freshen dynamic menu item properties
        //
        Board* board = registry->getGame().getBoard();
        TO_QACTION(mWhatsThisAction).setVisible( globalPos != 0 );
        TO_QACTION(mSpeedAction).setChecked( registry->getSpeedController().getHighSpeed() );
        TO_QACTION(mReloadAction).setData( QVariant( board->getLevel()) );

        PathSearchAction* actions[2];
        int nActions = 0;
        PieceType focus = registry->getMoveController().getFocus();
        if ( captureAction.setCriteria( focus, board->getFlagPoint() ) ) {
            actions[nActions++] = &captureAction;
        }
        if ( p.mCol < 0 ) {
            pathToAction.setVisible( false );
        } else if ( pathToAction.setCriteria( focus, p ) ) {
            actions[nActions++] = &pathToAction;
            pathToAction.setVisible( true );
        }
        registry->getPathFinderController().testActions( actions, nActions );

        bool movesPending = registry->getMoveController().getMoves().size() > 0;
        TO_QACTION(mUndoMoveAction).setEnabled( movesPending );
        TO_QACTION(mClearMovesAction).setEnabled( movesPending );
        bool modified = !registry->getRecorder().isEmpty();
        TO_QACTION(mReplayAction).setEnabled( modified || registry->getLevelList().isLevelCompleted( board->getLevel() ) );
        TO_QACTION(mReloadAction).setEnabled( modified );

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
        if ( action == &captureAction ) {
            registry->getPathFinderController().doAction( &captureAction );
        } else if ( action == &mWhatsThisAction ) {
            unsigned what = board->getPieceManager().typeAt( p );
            if ( what == NONE ) {
                if ( registry->getTank().getPoint().equals( p ) ) {
                    what = TANK;
                } else {
                    what = board->tileAt( p );
                    switch( what ) {
                    case STONE_MIRROR__90:
                    case STONE_MIRROR_180:
                    case STONE_MIRROR_270:
                        what = STONE_MIRROR;
                        break;
                    default:
                        ;
                    }
                }
            }
            whatsthis( globalPos, what, registry, this );
        } else if ( action == &pathToAction ) {
            registry->getPathFinderController().doAction( &pathToAction );
        } else if ( action == &mReloadAction ) {
            loadLevel( board->getLevel() );
        }
    }
}

void BoardWindow::keyPressEvent(QKeyEvent *ev)
{
    if ( !ev->isAutoRepeat() ) {
        if ( GameRegistry* registry = getRegistry(this) ) {
            switch( ev->key() ) {
            case Qt::Key_Escape:
                if ( !ev->modifiers() && !checkForReplay() )  {
                    registry->getMoveController().dragStop();
                    showMenu();
                }
                break;

            case Qt::Key_Control:
                registry->getMoveController().setFocus( TANK );
                registry->getTank().pause();
                break;

            case Qt::Key_Space:
                if ( !checkForReplay() )  {
                    registry->getMoveController().fire();
                }
                break;

            case Qt::Key_C: // attempt to capture the flag
                if ( !checkForReplay() && registry->getMoveController().getDragState() == Inactive )  {
                    PathSearchAction& captureAction = registry->getCaptureAction();
                    Board* board = registry->getGame().getBoard();
                    captureAction.setCriteria( registry->getMoveController().getFocus(), board->getFlagPoint() );
                    registry->getPathFinderController().doAction( &captureAction );
                }
                break;

            default:
                int rotation = keyToAngle(ev->key());
                if ( rotation >= 0 ) {
                    registry->getMoveController().move( rotation );
                } else if ( ev->key() >= Qt::Key_0 && ev->key() <= Qt::Key_9 && !checkForReplay() ) {
                    registry->getMoveController().fire( ev->key() - Qt::Key_0 );
                }
            }
        }
    }
}

void BoardWindow::keyReleaseEvent( QKeyEvent* ev )
{
    if ( !ev->isAutoRepeat() ) {
        if ( GameRegistry* registry = getRegistry(this) ) {
            switch( ev->key() ) {
            case Qt::Key_Space:
                if ( !checkForReplay() )  {
                    registry->getTank().ceaseFire();
                }
                break;

            case Qt::Key_Control:
            {   MoveController& moveController = registry->getMoveController();
                moveController.setFocus( MOVE );
                registry->getTank().resume();
            }
                break;

            case Qt::Key_S:
                registry->getSpeedController().toggleHighSpeed();
                break;

            case Qt::Key_A:
                if ( ev->modifiers() == Qt::AltModifier ) {
                    registry->getGame().replayLevel();
                }
                break;

            case Qt::Key_L:
                if ( ev->modifiers() == Qt::AltModifier ) {
                    chooseLevel();
                }
                break;

            case Qt::Key_R:
                if ( ev->modifiers() == Qt::AltModifier ) {
                    registry->getGame().restartLevel();
                }
                break;

            case Qt::Key_Backspace:
                if ( !checkForReplay() ) {
                    registry->getMoveController().undo();
                }
                break;

            case Qt::Key_Home:
                if ( !checkForReplay() && ev->modifiers() == Qt::ControlModifier ) {
                    registry->getMoveController().setFocus( TANK );
                }
                break;

            case Qt::Key_End:
                if ( !checkForReplay() && ev->modifiers() == Qt::ControlModifier ) {
                    registry->getMoveController().setFocus( MOVE );
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
}

void BoardWindow::mousePressEvent( QMouseEvent* event )
{
    switch( event->button() ) {
    case Qt::RightButton:
        if ( !checkForReplay() )  {
            QPoint globalPos = event->globalPos();
            showMenu( &globalPos, ModelPoint( event->pos() ) );
        }
        break;

    case Qt::LeftButton:
        if ( !checkForReplay() )  {
            if ( GameRegistry* registry = getRegistry(this) ) {
                registry->getMoveController().dragStart( ModelPoint( event->pos() ) );
            }
        }
        break;

    default:
        ;
    }
}

void BoardWindow::mouseReleaseEvent( QMouseEvent* event )
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        switch( event->button() ) {
        case Qt::LeftButton:
            registry->getMoveController().dragStop();
            break;
        default:
            ;
        }
    }
}

void BoardWindow::mouseMoveEvent(QMouseEvent* event)
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        registry->getMoveController().onDragTo( event->pos() );
    }
}

#ifndef Q_OS_WIN
static void drawForbidden( QPen& pen, int width, int height, QBitmap* b )
{
    QPainter painter(b);
    painter.setPen( pen );
    int w = width -2;
    int h = height-2;
    painter.drawArc( 1, 1, w, h, 0, 360*16 );
    painter.drawLine( 1,h, w,1 );
}
#endif // Q_OS_WIN

void BoardWindow::setCursorDragState( DragState state )
{
    mDragMarker.disable();

    switch( state ) {
    case DraggingTank:
        setCursor( Qt::CrossCursor );
        break;
    case DraggingTile:
        if ( GameRegistry* registry = getRegistry(this) ) {
            MoveController& moveController = registry->getMoveController();
            if ( int mask = moveController.getDragTileAngleMask() ) {
                setCursor( Qt::CrossCursor );
                mDragMarker.enable( mask, moveController.getDragTilePoint().toViewCenterSquare(), TILE_SIZE,
                  moveController.getTileDragFocusAngle() );
            }
        }
        break;
    case Forbidden:
    case ForbiddenTank:
    case ForbiddenTile:
        if ( !mForbiddenCursor ) {
#ifdef Q_OS_WIN
            mForbiddenCursor = new QCursor( Qt::ForbiddenCursor );
#else
            QSize bitmapSize(32,32);
            const int cursorWidth  = TILE_SIZE-4;
            const int cursorHeight = TILE_SIZE-4;
            QPen pen;

            QBitmap bitmap(bitmapSize);
            bitmap.clear();
            pen.setWidth(5);
            drawForbidden( pen, cursorWidth, cursorHeight, &bitmap );
            pen.setWidth(2);
            pen.setColor( Qt::color0 );
            drawForbidden( pen, cursorWidth, cursorHeight, &bitmap );

            QBitmap mask(bitmapSize);
            mask.clear();
            pen.setWidth(5);
            pen.setColor( Qt::color1 );
            drawForbidden( pen, cursorWidth, cursorHeight, &mask );

            mForbiddenCursor = new QCursor( bitmap, mask, cursorWidth/2, cursorHeight/2 );
#endif // Q_OS_WIN
        }
        setCursor( *mForbiddenCursor );
        break;
    default:
        unsetCursor();
    }
}

void BoardWindow::onRecordedCountChanged()
{
    mStatusDirty = true;
}

int BoardWindow::checkForReplay()
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        MoveController& moveController = registry->getMoveController();
        if ( moveController.replaying() ) {
            registry->getTank().pause();

            QMessageBox::StandardButton button = QMessageBox::question( 0, "Auto Replay", "Play from here?",
                                                                        QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes );
            if ( button == QMessageBox::Yes ) {
                moveController.setReplay( false );
            }
            if ( moveController.canWakeup() ) {
                registry->getTank().resume();
            }

            return (button == QMessageBox::Yes) ? -1 /* indicate changed to inactive */ : 1 /* indicate replay is active */;
        }
    }

    return 0; // indicate replay inactive
}

void BoardWindow::connectTo( const PieceManager& manager ) const
{
    QObject::connect( &manager, &PieceManager::insertedAt, this, &BoardWindow::renderSquareLater, Qt::DirectConnection );
    QObject::connect( &manager, &PieceManager::erasedAt,   this, &BoardWindow::renderSquareLater, Qt::DirectConnection );
    QObject::connect( &manager, &PieceManager::changedAt,  this, &BoardWindow::renderSquareLater, Qt::DirectConnection );
}

#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
void BoardWindow::requestUpdate()
{
    if ( isExposed() && !mUpdatePending ) {
        QTimer::singleShot( 17, this, &BoardWindow::renderNow );
        mUpdatePending = true;
    }
}
#endif

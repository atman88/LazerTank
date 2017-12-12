#include <QMenu>
#include <QLayout>
#include <QApplication>
#include <QAction>
#include <QStatusBar>
#include <QLabel>
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

class WhatsThisAwareLabel : public QLabel
{
public:
    WhatsThisAwareLabel( QWidget* parent = 0 );

protected:
    void mousePressEvent( QMouseEvent* event );

private:
    QAction mAction;
};


/**
 * @brief Confirms whether replay is active
 * @return 0 if not active, 1 if active or -1 if set inactive as a result of this call
 */
int checkForReplay( GameRegistry* registry );

BoardWidget::BoardWidget(QWidget* parent) : QWidget(parent), mForbiddenCursor(0)
{
    setAttribute( Qt::WA_OpaquePaintEvent );
    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
}

BoardWidget::~BoardWidget()
{
    if ( mForbiddenCursor ) {
        delete mForbiddenCursor;
    }
}

void BoardWidget::init( GameRegistry* registry )
{
    Game& game = registry->getGame();
    QObject::connect( &game, &Game::boardLoaded, this, &BoardWidget::onBoardLoaded, Qt::DirectConnection );

    Board* board = game.getBoard();
    QObject::connect( board, &Board::tileChangedAt, this, &BoardWidget::renderSquareLater, Qt::DirectConnection );

    PieceSetManager& pm = board->getPieceManager();
    QObject::connect( &pm, &PieceSetManager::erasedAt,   this, &BoardWidget::renderSquareLater, Qt::DirectConnection );
    QObject::connect( &pm, &PieceSetManager::insertedAt, this, &BoardWidget::renderSquareLater, Qt::DirectConnection );

    MoveController& moveController = registry->getMoveController();
    QObject::connect( &moveController, &MoveController::dragStateChanged, this, &BoardWidget::setCursorDragState );
    QObject::connect( &moveController, &MoveController::tileDragFocusChanged, &mDragMarker, &TileDragMarker::setFocus );
    QObject::connect( &mDragMarker, &TileDragMarker::rectDirty, this, &BoardWidget::renderLater, Qt::DirectConnection );

    Tank& tank = registry->getTank();
    QObject::connect( &tank,                                  &Tank::changed,       this, &BoardWidget::renderLater, Qt::DirectConnection );
    QObject::connect( &registry->getTank().getShot(),         &ShotView::rectDirty, this, &BoardWidget::renderLater, Qt::DirectConnection );
    QObject::connect( &registry->getActiveCannon().getShot(), &ShotView::rectDirty, this, &BoardWidget::renderLater, Qt::DirectConnection );
    QObject::connect( &registry->getTankPush(),               &Push::rectDirty,     this, &BoardWidget::renderLater, Qt::DirectConnection );
    QObject::connect( &registry->getShotPush(),               &Push::rectDirty,     this, &BoardWidget::renderLater, Qt::DirectConnection );
    QObject::connect( &moveController.getFutureShots(), &FutureShotPathManager::dirtyRect, this, &BoardWidget::renderLater, Qt::DirectConnection );

    mWhatsThisAction.setText( "What's this?" );
}

void BoardWidget::paintEvent( QPaintEvent* e )
{
    if ( isVisible() ) {
        if ( GameRegistry* registry = getRegistry(this) ) {
            QPainter painter(this);

            Board* board = registry->getGame().getBoard();
            Tank& tank = registry->getTank();
            MoveController& moveController = registry->getMoveController();

            if ( registry->getGame().isBoardLoaded() ) {
                BoardRenderer renderer(TILE_SIZE);
                renderer.render( e->rect(), board, &painter );

                bool tankIsProminent = moveController.getFocus() != TANK && moveController.getDragState() == Inactive;
                if ( tankIsProminent && moveController.getDragState() != Inactive ) {
                    if ( Piece* piece = moveController.getMoves().getBack() ) {
                        tankIsProminent = !tank.getPoint().equals( *piece );
                    }
                }

                if ( tankIsProminent ) {
                    // render the moves beneath (i.e. before) the tank:
                    renderer.renderMoves( e->rect(), registry, &painter );
                }

                registry->getTankPush().render( &e->rect(), renderer, &painter );
                registry->getShotPush().render( &e->rect(), renderer, &painter );
                tank.render( &e->rect(), &painter );

                if ( !tankIsProminent ) {
                    // render the moves on top of (i.e. after) the tank:
                    renderer.renderMoves( e->rect(), registry, &painter );
                }

                mDragMarker.render( &e->rect(), &painter );
                registry->getCannonShot().render( &painter );
            }
        }
    }
}

QSize BoardWidget::sizeHint() const
{
    int width = 6, height = 6;
    if ( GameRegistry* registry = getRegistry(this) ) {
        Board* board = registry->getGame().getBoard();
        width  = std::max( width,  board->getWidth()  );
        height = std::max( height, board->getHeight() );
    }
    return QSize( width * TILE_SIZE, height * TILE_SIZE );
}

void BoardWidget::onBoardLoaded()
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        Board* board = registry->getGame().getBoard();
        int w = board->getWidth() * TILE_SIZE;
        int h = board->getHeight()  * TILE_SIZE;
        setFixedSize( w, h );
        update( 0, 0, w, h );
    }
}

void BoardWidget::renderLater( const QRect& rect )
{
    update( rect );
}

void BoardWidget::renderSquareLater( ModelPoint point )
{
    update(point.mCol*TILE_SIZE, point.mRow*TILE_SIZE, TILE_SIZE, TILE_SIZE);
}

BoardWindow::BoardWindow(QWidget* parent) : QMainWindow(parent), mMoveCounter(0), mGameInitialized(false), mHelpWidget(0), mReplayText(0)
#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
  , mUpdatePending(false)
#endif
{
    setCentralWidget( new BoardWidget(this) );
    layout()->setSizeConstraint(QLayout::SetFixedSize);
}

BoardWindow::~BoardWindow()
{
    if ( mHelpWidget ) {
        delete mHelpWidget;
    }
    if ( mReplayText ) {
        delete mReplayText;
    }
}

void BoardWindow::init( GameRegistry* registry )
{
    Game& game = registry->getGame();
    MoveController& moveController = registry->getMoveController();

    TO_QACTION(mSpeedAction).setCheckable(true);

    QObject::connect( &TO_QACTION(mSpeedAction), &QAction::toggled, &registry->getSpeedController(), &SpeedController::setHighSpeed );
    QObject::connect( &TO_QACTION(mUndoMoveAction),  &QAction::triggered, &moveController, &MoveController::undoLastMove );
    QObject::connect( &TO_QACTION(mClearMovesAction),&QAction::triggered, &moveController, &MoveController::undoMoves );
    QObject::connect( &TO_QACTION(mReplayAction),    &QAction::triggered, &game, &Game::replayLevel );

    QObject::connect( &game, &Game::boardLoaded, this, &BoardWindow::onBoardLoaded, Qt::DirectConnection );

    Tank& tank = registry->getTank();
    QObject::connect( &mMenu, &QMenu::aboutToShow, &tank, &Tank::pause  );
    QObject::connect( &mMenu, &QMenu::aboutToHide, &tank, &Tank::resume );

    static_cast<BoardWidget*>( centralWidget() )->init( registry );

    Recorder& recorder = registry->getRecorder();
    QObject::connect( &recorder, &Recorder::recordedCountChanged, this, &BoardWindow::onRecordedCountChanged );
    mMoveCounter = new WhatsThisAwareLabel( this );
    mMoveCounter->setAlignment( Qt::AlignLeft );
    mMoveCounter->setNum( recorder.getRecordedCount() );
    mMoveCounter->setWhatsThis( "The number of moves taken" );
    statusBar()->addWidget( mMoveCounter );
}

void BoardWindow::onBoardLoaded()
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        int level = registry->getGame().getBoard()->getLevel();
        if ( level > 0 ) {
            QString title( QString("Level %1").arg(level) );
            setWindowTitle( title );
        }
    }
}

void BoardWindow::showHelp()
{
    if ( !mHelpWidget ) {
        mHelpWidget = new QTextBrowser();
        mHelpWidget->setWindowTitle( QString("LazerTank Help") );
        mHelpWidget->setGeometry( QRect(50,50,myScreenWidth()/2, (myScreenHeight()*2)/3) );
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

        QSize s = chooser->preferredSize();
        chooser->setGeometry( 0, 0, s.width(), std::min( myScreenHeight(), s.height() ) );
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

QMenu& BoardWindow::getMenu()
{
    return mMenu;
}

//void BoardWindow::renderBoard( const QRect* rect, GameRegistry* registry, QPainter* painter )
//{
//    if ( moveController.replaying() ) {
//        if ( !mReplayText ) {
//            mReplayText = new ReplayText( this, QString("REPLAY") );
//            QObject::connect( mReplayText, &ReplayText::dirty, this, &BoardWindow::renderLater, Qt::DirectConnection );
//        }

//        mReplayText->render( rect, painter );
//    }
//}

bool BoardWindow::resizeInternal( const QSize& /*size*/ )
{
    if ( mReplayText ) {
        mReplayText->onResize();
    }
    return true;
}

void BoardWindow::resizeEvent( QResizeEvent* resizeEvent )
{
    resizeInternal( resizeEvent->size() );
}

bool BoardWidget::event( QEvent* event )
{
    QEvent::Type type = event->type();
    if ( type == QltWhatsThisEvent::getEventType() ) {
        QltWhatsThisEvent* whatsThisEvent = static_cast<QltWhatsThisEvent*>( event );
        QWhatsThis::showText( whatsThisEvent->getPos(), whatsThisEvent->getHelpText() );
        return true;
    }

    return QWidget::event(event);
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

QAction* BoardWindow::showMenu( QPoint* globalPos, const QList<QAction*> widgetActions, const QList<PathSearchAction*> widgetSearchActions )
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        QPoint pos;

        PathSearchAction& captureAction = registry->getCaptureAction();

        //
        // create the menu on first use
        //
        if ( mMenu.isEmpty() ) {
            TO_QACTION(mUndoMoveAction).setText( "&Undo" );
            TO_QACTION(mClearMovesAction).setText( "Clear Moves" );
            TO_QACTION(mSpeedAction).setText( "&Speed Boost" );
            captureAction.setText( "&Capture Flag" );
            TO_QACTION(mReloadAction).setText( "&Restart Level" );
            TO_QACTION(mReplayAction).setText( "&Auto Replay" );

            TO_QACTION(mSpeedAction).setShortcut( Qt::Key_S );
            captureAction.setShortcut( Qt::Key_C );
            TO_QACTION(mUndoMoveAction).setShortcut( Qt::Key_Backspace );
            TO_QACTION(mClearMovesAction).setShortcut( Qt::CTRL|Qt::Key_Backspace );
            TO_QACTION(mReloadAction).setShortcut( Qt::ALT|Qt::Key_R );
            TO_QACTION(mReplayAction).setShortcut( Qt::ALT|Qt::Key_A );

            mMenu.addSeparator(); // separate contextual actions (above) and non-contextual (below)
            mMenu.addAction( "shoot& ", &registry->getMoveController(), SLOT(fire()), Qt::Key_Space );
            mMenu.addAction( &TO_QACTION(mSpeedAction)      );
            mMenu.addAction( &TO_QACTION(mUndoMoveAction)   );
            mMenu.addAction( &TO_QACTION(mClearMovesAction) );
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
        TO_QACTION(mSpeedAction).setChecked( registry->getSpeedController().getHighSpeed() );
        TO_QACTION(mReloadAction).setData( QVariant( board->getLevel()) );

        QList<PathSearchAction*> searchActions( widgetSearchActions );
        PieceType focus = registry->getMoveController().getFocus();
        if ( captureAction.setCriteria( focus, board->getFlagPoint() ) ) {
            searchActions.append( &captureAction );
        }
        registry->getPathFinderController().testActions( searchActions );

        bool movesPending = registry->getMoveController().getMoves().size() > 0;
        TO_QACTION(mUndoMoveAction).setEnabled( movesPending );
        TO_QACTION(mClearMovesAction).setEnabled( movesPending );
        bool modified = !registry->getRecorder().isEmpty();
        TO_QACTION(mReplayAction).setEnabled( modified || registry->getLevelList().isLevelCompleted( board->getLevel() ) );
        TO_QACTION(mReloadAction).setEnabled( modified );

        mMenu.insertActions( mMenu.actions().at(0), widgetActions );

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
        } else if ( action == &mReloadAction ) {
            loadLevel( board->getLevel() );
        }

        for( QAction* a : widgetActions ) {
            mMenu.removeAction( a );
        }
        return action;
    }

    return 0;
}

void BoardWindow::keyPressEvent(QKeyEvent *ev)
{
    if ( !ev->isAutoRepeat() ) {
        if ( GameRegistry* registry = getRegistry(this) ) {
            switch( ev->key() ) {
            case Qt::Key_Escape:
                if ( !ev->modifiers() && !checkForReplay(registry) )  {
                    registry->getMoveController().dragStop();
                    showMenu();
                }
                break;

            case Qt::Key_Control:
                registry->getMoveController().setFocus( TANK );
                registry->getTank().pause();
                break;

            case Qt::Key_Space:
                if ( !checkForReplay(registry) )  {
                    registry->getMoveController().fire();
                }
                break;

            case Qt::Key_C: // attempt to capture the flag
                if ( !checkForReplay(registry) && registry->getMoveController().getDragState() == Inactive )  {
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
                } else if ( ev->key() >= Qt::Key_0 && ev->key() <= Qt::Key_9 && !checkForReplay(registry) ) {
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
                if ( !checkForReplay(registry) )  {
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
                if ( !checkForReplay(registry) ) {
                    registry->getMoveController().undo();
                }
                break;

            case Qt::Key_Home:
                if ( !checkForReplay(registry) && ev->modifiers() == Qt::ControlModifier ) {
                    registry->getMoveController().setFocus( TANK );
                }
                break;

            case Qt::Key_End:
                if ( !checkForReplay(registry) && ev->modifiers() == Qt::ControlModifier ) {
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

void BoardWidget::mousePressEvent( QMouseEvent* event )
{
    switch( event->button() ) {
    case Qt::RightButton:
        if ( GameRegistry* registry = getRegistry(this) ) {
            if ( !checkForReplay(registry) )  {
                Board* board = registry->getGame().getBoard();
                PathSearchAction& pathToAction = registry->getPathToAction();

                QList<QAction*> myActions( { &mWhatsThisAction } );

                QPoint globalPos = event->globalPos();
                ModelPoint p( event->pos() );
                if ( p.mCol >= 0 && pathToAction.setCriteria( registry->getMoveController().getFocus(), p ) ) {
                    myActions.append( &pathToAction );
                }

                QAction* action = registry->getWindow()->showMenu( &globalPos, myActions, { &pathToAction } );

                if ( action == &pathToAction ) {
                    registry->getPathFinderController().doAction( &pathToAction );
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
                            case STONE_SLIT_90:
                                what = STONE_SLIT;
                                break;
                            default:
                                ;
                            }
                        }
                    }
                    whatsthis( &globalPos, what, registry, this );
                }
            }
        }
        break;

    case Qt::LeftButton:
        if ( GameRegistry* registry = getRegistry(this) ) {
            if ( !checkForReplay(registry) )  {
                registry->getMoveController().dragStart( ModelPoint( event->pos() ) );
            }
        }
        break;

    default:
        ;
    }
}

void BoardWidget::mouseReleaseEvent( QMouseEvent* event )
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

void BoardWidget::mouseMoveEvent(QMouseEvent* event)
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

void BoardWidget::setCursorDragState( DragState state )
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
    if ( GameRegistry* registry = getRegistry(this) ) {
        mMoveCounter->setNum( registry->getRecorder().getRecordedCount() );
    }
}

int checkForReplay( GameRegistry* registry )
{
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

    return 0; // indicate replay inactive
}

void BoardWindow::connectTo( const PieceManager& manager ) const
{
    BoardWidget* w = static_cast<BoardWidget*>( centralWidget() );
    QObject::connect( &manager, &PieceManager::insertedAt, w, &BoardWidget::renderSquareLater, Qt::DirectConnection );
    QObject::connect( &manager, &PieceManager::erasedAt,   w, &BoardWidget::renderSquareLater, Qt::DirectConnection );
    QObject::connect( &manager, &PieceManager::changedAt,  w, &BoardWidget::renderSquareLater, Qt::DirectConnection );
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

WhatsThisAwareLabel::WhatsThisAwareLabel( QWidget* parent ) : QLabel(parent)
{
    mAction.setText( "What's this?" );
}

void WhatsThisAwareLabel::mousePressEvent(QMouseEvent *event)
{
    if ( event->button() == Qt::RightButton ) {
        if ( GameRegistry* registry = getRegistry(this) ) {
            QPoint globalPos = event->globalPos();
            QAction* action = registry->getWindow()->showMenu( &globalPos, { &mAction } );
            if ( action == &mAction ) {
                QWhatsThis::showText( globalPos, whatsThis() );
            }
        }
    }
}

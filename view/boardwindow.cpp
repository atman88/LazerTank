#include <QMenu>
#include <QLayout>
#include <QApplication>
#include <QStatusBar>
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


/**
 * @brief Confirms whether replay is active
 * @return 0 if not active, 1 if active or -1 if set inactive as a result of this call
 */
int checkForReplay( GameRegistry* registry );

BoardWidget::BoardWidget(QWidget* parent) : QWidget(parent), mForbiddenCursor{nullptr}
{
    setAttribute( Qt::WA_OpaquePaintEvent );
    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
}

BoardWidget::~BoardWidget()
{
    delete mForbiddenCursor;
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
                painter.fillRect( e->rect(), Qt::black );
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

                registry->getTankPush().render( &e->rect(), &painter );
                registry->getShotPush().render( &e->rect(), &painter );
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
    return { width * TILE_SIZE, height * TILE_SIZE };
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

BoardWindow::BoardWindow(QWidget* parent) : QMainWindow(parent), mMoveCounter(new WhatsThisAwareLabel(this)),
  mSavedMoveCount(new WhatsThisAwareLabel(this)), mCompletedIndicator(new WhatsThisAwareLabel(this)),
  mGameInitialized{false}, mHelpWidget{nullptr}, mReplayText{nullptr}, mBackdoorCode{0}
{
    setCentralWidget( new BoardWidget(this) );
    layout()->setSizeConstraint(QLayout::SetFixedSize);
}

BoardWindow::~BoardWindow()
{
    delete mHelpWidget;
    delete mReplayText;
}

void BoardWindow::init( GameRegistry* registry )
{
    Game& game = registry->getGame();
    MoveController& moveController = registry->getMoveController();

    mSpeedAction.setCheckable(true);

    QObject::connect( &mSpeedAction, &QAction::toggled, &registry->getSpeedController(), &SpeedController::setHighSpeed );
    QObject::connect( &mUndoMoveAction,  &QAction::triggered, &moveController, &MoveController::undoLastMove );
    QObject::connect( &mClearMovesAction,&QAction::triggered, &moveController, &MoveController::undoMoves    );
    QObject::connect( &mReplayAction,    &QAction::triggered, &game, &Game::replayLevel );

    QObject::connect( &game, &Game::boardLoaded, this, &BoardWindow::onBoardLoaded, Qt::DirectConnection );

    Tank& tank = registry->getTank();
    QObject::connect( &mMenu, &QMenu::aboutToShow, &tank, &Tank::pause  );
    QObject::connect( &mMenu, &QMenu::aboutToHide, &tank, &Tank::resume );

    static_cast<BoardWidget*>( centralWidget() )->init( registry );

    if ( QStatusBar* status = statusBar() ) {
        status->setStyleSheet(
            "* {"
               "background-color: black;"
               "font: bold;"
            "}" );
        status->setSizeGripEnabled(false);

        ReplayText* replayText = new ReplayText( this, "REPLAY" );
        replayText->setVisible(false);
        QObject::connect( &moveController, &MoveController::replayChanged, replayText, &QLabel::setVisible, Qt::QueuedConnection );
        replayText->setWhatsThis( "Auto Replay mode indicator" );
        status->addWidget( replayText );

        Recorder& recorder = registry->getRecorder();
        QObject::connect( &recorder, &Recorder::recordedCountChanged, this, &BoardWindow::onRecordedCountChanged );
        mMoveCounter->setAlignment( Qt::AlignLeft|Qt::AlignHCenter );
        mMoveCounter->setStyleSheet( "* { color: gray; }" );
        mMoveCounter->setNum( recorder.getRecordedCount() );
        mMoveCounter->setWhatsThis( "Current move count. Shows the number of moves taken so far" );
        status->addWidget( mMoveCounter );

        QObject::connect( &registry->getLevelList(), &LevelList::levelUpdated, this, &BoardWindow::onLevelUpdated );
        if( const QPixmap* pm = ResourcePixmap::getPixmap(COMPLETE_CHECKMARK) ) {
            mCompletedIndicator->setPixmap( *pm );
            mCompletedIndicator->setWhatsThis( "Completed indicator. Shows this level has been successfully completed before" );
            status->addPermanentWidget( mCompletedIndicator );
        }

        mSavedMoveCount->setAlignment( Qt::AlignRight|Qt::AlignHCenter );
        mSavedMoveCount->setStyleSheet( "* { color: gray; }" );
        mSavedMoveCount->setWhatsThis( "Completed count. The total moves taken when this level was last completed" );
        status->addPermanentWidget( mSavedMoveCount );
    }
}

void BoardWindow::onBoardLoaded()
{
    bool completedVisible = false;

    if ( GameRegistry* registry = getRegistry(this) ) {
        int levelNo = registry->getGame().getBoard()->getLevel();
        if ( levelNo > 0 ) {
            QString title( QString("Level %1").arg(levelNo) );
            setWindowTitle( title );

            if ( const Level* level = registry->getLevelList().find( levelNo ) ) {
                if ( int completedCount = level->getCompletedCount() ) {
                    mSavedMoveCount->setNum( completedCount );
                    completedVisible = true;
                }
            }
        }
    }

    mSavedMoveCount->setVisible(completedVisible);
    mCompletedIndicator->setVisible(completedVisible);
}

void BoardWindow::onLevelUpdated( const QModelIndex& index )
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        if ( const Level* level = registry->getLevelList().at( index.row() ) ) {
            if ( level->getNumber() == registry->getGame().getBoard()->getLevel() ) {
                bool completedVisible = false;
                if ( int completedCount = level->getCompletedCount() ) {
                    mSavedMoveCount->setNum( completedCount );
                    completedVisible = true;
                }
                mSavedMoveCount->setVisible(completedVisible);
                mCompletedIndicator->setVisible(completedVisible);
            }
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
        auto chooser = new LevelChooser( registry->getLevelList(), registry->getBoardPool() );
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
//        }

//    }
//}

bool BoardWidget::event( QEvent* event )
{
    QEvent::Type type = event->type();
    if ( type == QltWhatsThisEvent::getEventType() ) {
        auto whatsThisEvent = static_cast<QltWhatsThisEvent*>( event );
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

QAction* BoardWindow::showMenu( QPoint* globalPos, const QList<QAction*> widgetActions, const QList<PathSearchAction*>& widgetSearchActions )
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        QPoint pos;

        PathSearchAction& captureAction = registry->getCaptureAction();

        //
        // create the menu on first use
        //
        if ( mMenu.isEmpty() ) {
            mUndoMoveAction.setText( "&Undo" );
            mClearMovesAction.setText( "Clear Moves" );
            mSpeedAction.setText( "&Speed Boost" );
            captureAction.setText( "&Capture Flag" );
            mReloadAction.setText( "&Restart Level" );
            mReplayAction.setText( "&Auto Replay" );

            mSpeedAction.setShortcut( Qt::Key_S );
            captureAction.setShortcut( Qt::Key_C );
            mUndoMoveAction.setShortcut( Qt::Key_Backspace );
            mClearMovesAction.setShortcut( Qt::CTRL|Qt::Key_Backspace );
            mReloadAction.setShortcut( Qt::ALT|Qt::Key_R );
            mReplayAction.setShortcut( Qt::ALT|Qt::Key_A );

            mMenu.addSeparator(); // separate contextual actions (above) and non-contextual (below)
            mMenu.addAction( "shoot& ", &registry->getMoveController(), SLOT(fire()), Qt::Key_Space );
            mMenu.addAction( &mSpeedAction      );
            mMenu.addAction( &mUndoMoveAction   );
            mMenu.addAction( &mClearMovesAction );
            mMenu.addAction( &captureAction );
            mMenu.addAction( &mReloadAction     );
            mMenu.addAction( "Select &Level..", this, SLOT(chooseLevel()), Qt::ALT|Qt::Key_L );
            mMenu.addAction( &mReplayAction );
            mMenu.addAction( "&Help", this, SLOT(showHelp()) );
            mMenu.addAction( "About Qt", qApp, &QApplication::aboutQt );
            mMenu.addAction( "E&xit", this, SLOT(close()) );
        }

        //
        // freshen dynamic menu item properties
        //
        Board* board = registry->getGame().getBoard();
        mSpeedAction.setChecked( registry->getSpeedController().getHighSpeed() );
        mReloadAction.setData( QVariant( board->getLevel()) );

        QList<PathSearchAction*> searchActions( widgetSearchActions );
        PieceType focus = registry->getMoveController().getFocus();
        if ( captureAction.setCriteria( focus, board->getFlagPoint() ) ) {
            searchActions.append( &captureAction );
        }
        registry->getPathFinderController().testActions( searchActions );

        bool movesPending = registry->getMoveController().getMoves().size() > 0;
        mUndoMoveAction.setEnabled( movesPending );
        mClearMovesAction.setEnabled( movesPending );
        bool modified = !registry->getRecorder().isEmpty();
        mReplayAction.setEnabled( modified || registry->getLevelList().isLevelCompleted( board->getLevel() ) );
        mReloadAction.setEnabled( modified );

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

    return nullptr;
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

            case Qt::Key_Shift: // start of a new code
                mBackdoorCode = 0;
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
    // handle backdoor
    if ( ev->modifiers() & Qt::ShiftModifier ) {
        int key = ev->key();
        if ( Qt::Key_A <= key && key <= Qt::Key_Z )
            mBackdoorCode = (mBackdoorCode << 8 ) | key;
        else
            mBackdoorCode = 0;
        return;
    }

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

            case Qt::Key_D: // dump recording (debug)
                if ( ev->modifiers() == Qt::AltModifier ) {
                    registry->getRecorder().dump();
                }
                break;

            case Qt::Key_Shift:
                if ( mBackdoorCode > 0xff )
                    emit backdoor( mBackdoorCode );
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

void BoardWindow::connectTo( const PieceManager& manager ) const
{
    auto w = static_cast<BoardWidget*>( centralWidget() );
    QObject::connect( &manager, &PieceManager::insertedAt, w, &BoardWidget::renderSquareLater, Qt::DirectConnection );
    QObject::connect( &manager, &PieceManager::erasedAt,   w, &BoardWidget::renderSquareLater, Qt::DirectConnection );
    QObject::connect( &manager, &PieceManager::changedAt,  w, &BoardWidget::renderSquareLater, Qt::DirectConnection );
}

#include <iostream>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QTextBrowser>

#include "boardwindow.h"
#include "replaytext.h"
#include "util/renderutils.h"
#include "util/imageutils.h"
#include "controller/game.h"
#include "controller/gameregistry.h"


BoardWindow::BoardWindow(QWindow *parent) : QWindow(parent), mBackingStore(0), mRenderedOnce(false), mFocus(MOVE),
  mGameInitialized(false), mHelpWidget(0), mReplayText(0)
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
    delete mBackingStore;
}

void BoardWindow::setSize( int cols, int rows )
{
    QRect size( 0, 0, cols * 24, rows * 24 );
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
    renderNow();
}

void BoardWindow::init( GameRegistry* registry )
{
    Game& game = registry->getGame();
    setProperty("GameHandle", registry->property("GameHandle"));

    Board* board = game.getBoard();
    QObject::connect( board, &Board::tileChangedAt, this, &BoardWindow::renderSquareLater );

    PieceSetManager* pm = board->getPieceManager();
    QObject::connect( pm, &PieceSetManager::erasedAt,   this, &BoardWindow::renderSquareLater );
    QObject::connect( pm, &PieceSetManager::insertedAt, this, &BoardWindow::renderSquareLater );

    QObject::connect( &TO_QACTION(mSpeedAction), &QAction::toggled, &registry->getSpeedController(), &SpeedController::setHighSpeed );
    QObject::connect( &TO_QACTION(mUndoMoveAction),  &QAction::triggered, &game, &Game::undoLastMove );
    QObject::connect( &TO_QACTION(mClearMovesAction),&QAction::triggered, &registry->getMoveController(), &MoveController::clearMoves );
    QObject::connect( &TO_QACTION(mReplayAction),    &QAction::triggered, &game, &Game::replayLevel );

    QObject::connect( &game, &Game::boardLoaded, this, &BoardWindow::onBoardLoaded );
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

void BoardWindow::render( const QRect* rect, GameRegistry* registry, QPainter* painter )
{
    Board* board = registry->getGame().getBoard();
    MoveController& moveController = registry->getMoveController();
    const PieceMultiSet* moves = moveController.getMoves()->toMultiSet();
    const PieceSet* tiles = board->getPieceManager()->getPieces();
    const PieceSet* deltas = registry->getGame().getDeltaPieces();

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
    for( auto it : *moveController.getFutureShots()->getPaths() ) {
        if ( rect->intersects( it.getBounds() ) ) {
            if ( !usingFutureShotPen ) {
                QPen pen( registry->getTank().getShot().getPen() );

                // dim it's color to contrast future shots from actual shots:
                QColor color = pen.color();
                color.setAlpha(127);
                pen.setColor( color );

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

    registry->getTankPush().render( rect, painter );
    registry->getShotPush().render( rect, painter );
    registry->getTank().render( rect, painter );
    if ( mFocus == TANK ) {
        // render the moves ontop of (i.e. after) the tank and it's pushes when focus is at
        // the tank:
        renderListIn( moveIterator, moves->end(), rect, painter );
    }
    registry->getCannonShot().render( painter );

    if ( moveController.replaying() ) {
        if ( !mReplayText ) {
            mReplayText = new ReplayText( this, QString("REPLAY") );
            QObject::connect( mReplayText, &ReplayText::dirty, this, &BoardWindow::renderLater );
        }

        mReplayText->render( rect, painter );
    }
}

void BoardWindow::renderNow()
{
    if ( mBackingStore ) {
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

            mBackingStore->beginPaint( mRenderRegion );
            QPainter painter( device );

            if ( !boardLoaded ) {
                painter.fillRect( rect, Qt::black );
            } else {
                render( &rect, registry, &painter );
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
            QKeySequence shootSeq( Qt::Key_Space );
            QKeySequence undoSeq( Qt::Key_Backspace );
            QKeySequence clearSeq( Qt::CTRL|Qt::Key_Backspace );
            QKeySequence speedSeq( Qt::Key_S );
            QKeySequence captureSeq( Qt::Key_C );
            QKeySequence replaySeq( Qt::ALT|Qt::Key_A );

            mMenu.addAction( "shoot& ", &registry->getMoveController(), SLOT(fire()), shootSeq );
            TO_QACTION(mUndoMoveAction).setText("&Undo");
            TO_QACTION(mClearMovesAction).setText("Clear Moves");
            TO_QACTION(mSpeedAction).setText( "&Speed Boost" );
            pathToAction.setText( "Move &Here" );
            captureAction.setText( "&Capture Flag" );
            TO_QACTION(mReloadAction).setText( "&Restart Level" );
            TO_QACTION(mReplayAction).setText( "&Auto Replay" );

            TO_QACTION(mSpeedAction).setShortcut( speedSeq );
            TO_QACTION(mSpeedAction).setCheckable(true);
            captureAction.setShortcut( captureSeq );
            TO_QACTION(mUndoMoveAction).setShortcut( undoSeq );
            TO_QACTION(mClearMovesAction).setShortcut(clearSeq);
            TO_QACTION(mReplayAction).setShortcut( replaySeq );

            mMenu.addAction( &TO_QACTION(mSpeedAction)      );
            mMenu.addAction( &TO_QACTION(mUndoMoveAction)   );
            mMenu.addAction( &TO_QACTION(mClearMovesAction) );
            mMenu.addAction( &pathToAction );
            mMenu.addAction( &captureAction );
            mMenu.addAction( &TO_QACTION(mReloadAction)     );

            QAction* action = mMenu.addAction( QString("Select &Level..") );
            action->setMenu( &mLevelsMenu );
            QString text( "level %1" );
            for( int level = 1; level <= BOARD_MAX_LEVEL; ++level ) {
                action = mLevelsMenu.addAction( text.arg(level) );
                action->setData( QVariant(level) );
            }

            mMenu.addAction( &TO_QACTION(mReplayAction) );
            mMenu.addAction( QString("&Help"), this, SLOT(showHelp()) );
            mMenu.addAction( QString("E&xit"), this, SLOT(close()) );
        }

        //
        // freshen dynamic menu item properties
        //
        Board* board = registry->getGame().getBoard();
        TO_QACTION(mSpeedAction).setChecked( registry->getSpeedController().getHighSpeed() );
        TO_QACTION(mReloadAction).setData( QVariant( board->getLevel()) );

        PathSearchAction* actions[2];
        int nActions = 0;
        if ( captureAction.setCriteria( mFocus, board->getFlagPoint(), true ) ) {
            actions[nActions++] = &captureAction;
        }
        if ( p.mCol < 0 ) {
            pathToAction.setVisible( false );
        } else if ( pathToAction.setCriteria( mFocus, p, true ) ) {
            actions[nActions++] = &pathToAction;
            pathToAction.setVisible( true );
        }
        registry->getPathFinderController().testActions( actions, nActions );

        bool movesPending = registry->getMoveController().getMoves()->size() > 0;
        TO_QACTION(mUndoMoveAction).setEnabled( movesPending );
        TO_QACTION(mClearMovesAction).setEnabled( movesPending );
        TO_QACTION(mReplayAction).setEnabled( !registry->getTank().getRecorder().isEmpty() );

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
        } else if ( action == &pathToAction ) {
            registry->getPathFinderController().doAction( &pathToAction );
        } else if ( action ) {
            bool ok;
            int level = action->data().toInt( &ok );
            if ( ok && level > 0 ) {
                registry->getGame().getBoard()->load( action->data().toInt() );
            } else {
                action->trigger();
            }
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
                    showMenu();
                }
                break;

            case Qt::Key_Control:
                moveFocus( TANK );
                break;

            case Qt::Key_Space:
                if ( !checkForReplay() )  {
                    registry->getMoveController().fire();
                }
                break;

            case Qt::Key_C: // attempt to capture the flag
                if ( !checkForReplay() )  {
                    PathSearchAction& captureAction = registry->getCaptureAction();
                    Board* board = registry->getGame().getBoard();
                    captureAction.setCriteria( mFocus, board->getFlagPoint(), false );
                    registry->getPathFinderController().doAction( &captureAction );
                }
                break;

            default:
                if ( !ev->modifiers() ) {
                    int rotation = keyToAngle(ev->key());
                    if ( rotation >= 0 && !checkForReplay() ) {
                        registry->getMoveController().move( rotation );
                    } else if ( ev->key() >= Qt::Key_0 && ev->key() <= Qt::Key_9 && !checkForReplay() ) {
                        registry->getMoveController().fire( ev->key() - Qt::Key_0 );
                    }
                }
            }
        }
    }
}

void BoardWindow::keyReleaseEvent(QKeyEvent *ev)
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
                moveFocus( MOVE );
                break;

            case Qt::Key_S:
                TO_QACTION(mSpeedAction).toggle();
                break;

            case Qt::Key_C:
                if ( !checkForReplay() )  {
                    registry->getMoveController().wakeup();
                }
                break;

            case Qt::Key_A:
                if ( ev->modifiers() == Qt::AltModifier ) {
                    registry->getGame().replayLevel();
                }
                break;

            case Qt::Key_Backspace:
                if ( !checkForReplay() )  {
                    if ( ev->modifiers() == Qt::ControlModifier ) {
                        registry->getMoveController().clearMoves();
                    } else {
                        registry->getGame().undoLastMove();
                    }
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
    if ( GameRegistry* registry = getRegistry(this) ) {
        switch( event->button() ) {
        case Qt::RightButton:
            if ( !checkForReplay() )  {
                QPoint globalPos = event->globalPos();
                showMenu( &globalPos, ModelPoint( event->pos() ) );
            }
            break;

        case Qt::LeftButton:
            if ( !checkForReplay() )  {
                PathSearchAction& pathToAction = registry->getPathToAction();
                pathToAction.setCriteria( mFocus, ModelPoint( event->pos() ), false );
                registry->getPathFinderController().doAction( &pathToAction );
            }
            break;

        default:
            ;
        }
    }
}

void BoardWindow::mouseReleaseEvent( QMouseEvent* event )
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        switch( event->button() ) {
        case Qt::LeftButton:
            if ( !checkForReplay() )  {
                registry->getMoveController().wakeup();
            }
            break;
        default:
            ;
        }
    }
}

void BoardWindow::moveFocus( PieceType what )
{
    mFocus = what;
    emit focusChanged( what );
}

int BoardWindow::checkForReplay()
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        if ( registry->getMoveController().replaying() ) {
            registry->getTank().pause();

            QMessageBox::StandardButton button = QMessageBox::question( 0, "Auto Replay", "Play from here?",
                                                                        QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes );
            if ( button == QMessageBox::Yes ) {
                registry->getMoveController().setReplay( false );
            }
            registry->getTank().resume();

            return (button == QMessageBox::Yes) ? -1 /* indicate changed to inactive */ : 1 /* indicate replay is active */;
        }
    }

    return 0; // indicate replay inactive
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

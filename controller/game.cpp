#include <iostream>
#include <Qt>
#include <QVariant>
#include <QMessageBox>
#include <QPushButton>

#include "game.h"
#include "boardwindow.h"
#include "speedcontroller.h"
#include "util/renderutils.h"

Game::Game() : mWindow(0)
{
    mHandle.game = this;
    setProperty("GameHandle", QVariant::fromValue(mHandle));
}

void Game::init( BoardWindow* window )
{
    mWindow = window;

    mTankPush.setParent( this );
    mShotPush.setParent( this );
    mSpeedController.setParent( this );
    mMoveController.setParent( this );

    if ( window ) {
        window->init( this );

        QObject::connect( window, &BoardWindow::focusChanged, &mMoveController, &MoveController::setFocus );
        QObject::connect( &mTankPush, &Push::rectDirty, window, &BoardWindow::renderLater );
        QObject::connect( &mShotPush, &Push::rectDirty, window, &BoardWindow::renderLater );

        QObject::connect( &mTank, &Tank::changed, window, &BoardWindow::renderLater );
        QObject::connect( mMoveController.getFutureShots(), &FutureShotPathManager::dirtyRect, window, &BoardWindow::renderLater );

        QMenu& menu = window->getMenu();
        QObject::connect( &menu, &QMenu::aboutToShow, &mTank, &Tank::pause  );
        QObject::connect( &menu, &QMenu::aboutToHide, &mTank, &Tank::resume );

        PieceListManager* moveManager = mMoveController.getMoves();
        QObject::connect( moveManager, &PieceListManager::appended, window, &BoardWindow::renderSquareLater );
        QObject::connect( moveManager, &PieceListManager::erased,   window, &BoardWindow::renderSquareLater );
        QObject::connect( moveManager, &PieceListManager::changed,  window, &BoardWindow::renderSquareLater );

        QObject::connect( mFutureDelta.getPieceManager(), &PieceSetManager::erasedAt,   window, &BoardWindow::renderSquareLater );
        QObject::connect( mFutureDelta.getPieceManager(), &PieceSetManager::insertedAt, window, &BoardWindow::renderSquareLater );
        QObject::connect( mFutureDelta.getPieceManager(), &PieceSetManager::changedAt,  window, &BoardWindow::renderSquareLater );
    }

    mTank.init( this );

    mMoveController.init( this );
    QObject::connect( &mMoveController, &MoveController::pushingInto, this, &Game::onTankPushingInto );
    QObject::connect( &mMoveController, &MoveController::idle, this, &Game::endMoveDeltaTracking );
    QObject::connect( &mPathFinderController, &PathFinderController::pathFound, &mMoveController, &MoveController::onPathFound );

    mActiveCannon.init( this, CANNON, QColor(255,50,83) );

    mPathFinderController.init(this);

    mMoveAggregate.setObjectName("MoveAggregate");
    mShotAggregate.setObjectName("ShotAggregate");

    mTankPush.init( this );
    mShotPush.init( this );
    QObject::connect( &mTankPush, &Push::stateChanged, &mMoveAggregate, &AnimationStateAggregator::onStateChanged );
    QObject::connect( &mShotPush, &Push::stateChanged, &mShotAggregate, &AnimationStateAggregator::onStateChanged );
    QObject::connect( &mMoveAggregate, &AnimationStateAggregator::finished, this, &Game::onMoveAggregatorFinished, Qt::QueuedConnection );
    QObject::connect( &mShotAggregate, &AnimationStateAggregator::finished, this, &Game::sightCannons );

    mFutureDelta.init( &mBoard, &mFutureBoard );

    QObject::connect( &mBoard, &Board::tileChangedAt, this, &Game::onBoardTileChanged );
    QObject::connect( &mBoard, &Board::boardLoaded,   this, &Game::onBoardLoaded      );
}

void Game::onBoardLoaded()
{
    mFutureDelta.enable( false );
    mMoveAggregate.reset();
    mShotAggregate.reset();
    mTank.reset( mBoard.getTankStartPoint() );
    mActiveCannon.reset( NullPoint );
    mSpeedController.setHighSpeed(false);
    mMoveController.onBoardLoaded( &mBoard );
}

void Game::endMoveDeltaTracking()
{
    mFutureDelta.enable( false );
}

GameHandle Game::getHandle()
{
    return mHandle;
}

Board* Game::getBoard( bool futuristic )
{
    if ( futuristic && mFutureDelta.enabled() ) {
        return mFutureDelta.getFutureBoard();
    }
    return &mBoard;
}

bool Game::isMasterBoard( Board* board )
{
    return board == &mBoard;
}

Tank* Game::getTank()
{
    return &mTank;
}

Push& Game::getTankPush()
{
    return mTankPush;
}

Push& Game::getShotPush()
{
    return mShotPush;
}

bool Game::canMoveFrom( PieceType what, int angle, ModelPoint *point, Board* board, Piece **pushPiece ) {
    return getAdjacentPosition(angle, point) && canPlaceAt( what, *point, angle, board, pushPiece );
}

bool Game::canMoveFrom( PieceType what, int angle, ModelPoint *point, bool futuristic, Piece **pushPiece )
{
    if ( what != TANK && point->equals( mTank.getPoint() ) ) {
        return false;
    }

    if ( getAdjacentPosition( angle, point ) ) {
        return canPlaceAt( what, *point, angle, getBoard(futuristic), pushPiece );
    }
    return false;
}

bool canSightThru( Board* board, int col, int row )
{
    switch( board->tileAt( col, row ) ) {
    case DIRT:
    case TILE_SUNK:
        return board->getPieceManager()->typeAt( col, row ) == NONE;
    case WATER:
    case FLAG:
        return true;
    default:
        return false;
    }
}

void Game::sightCannons()
{
    // fire any cannon
    const PieceSet* pieces = mBoard.getPieceManager()->getPieces();
    bool sighted = false;
    int tankCol = mTank.getCol();
    int tankRow = mTank.getRow();
    int fireAngle, fireCol, fireRow;

    for( auto it = pieces->cbegin(); !sighted && it != pieces->cend(); ++it ) {
        if ( (*it)->getType() == CANNON ) {
            fireAngle = (*it)->getAngle();
            if ( tankCol == (*it)->getCol() ) {
                fireRow = (*it)->getRow();
                int dir;
                if ( fireAngle == 0 && tankRow < fireRow ) {
                    dir = -1;
                } else if ( fireAngle == 180 && tankRow > fireRow ) {
                    dir = 1;
                } else {
                    continue;
                }
                for( int row = fireRow+dir; ; row += dir ) {
                    if ( row == tankRow ) {
                        fireCol = tankCol;
                        sighted = true;
                        break;
                    }
                    if ( !canSightThru( &mBoard, tankCol, row ) ) {
                        break;
                    }
                }
            } else if ( tankRow == (*it)->getRow() ) {
                fireCol = (*it)->getCol();
                int dir;
                if ( fireAngle == 270 && tankCol < fireCol ) {
                    dir = -1;
                } else if ( fireAngle == 90 && tankCol > fireCol ) {
                    dir = 1;
                } else {
                    continue;
                }
                for( int col = fireCol+dir; ; col += dir ) {
                    if ( col == tankCol ) {
                        fireRow = tankRow;
                        sighted = true;
                        break;
                    }
                    if ( !canSightThru( &mBoard, col, tankRow ) ) {
                        break;
                    }
                }
            }
        }
    }

    if ( sighted ) {
        mActiveCannon.setViewX( fireCol*24 );
        mActiveCannon.setViewY( fireRow*24 );
        mActiveCannon.setViewRotation( fireAngle );
        mActiveCannon.fire();
    }
}

void Game::onPushed(PieceType type, int col, int row, int pieceAngle )
{
    mBoard.applyPushResult( type, col, row, pieceAngle );
}

void Game::onMoveAggregatorFinished()
{
    if ( !mFutureDelta.getPieceManager()->size() ) {
        mFutureDelta.enable( false );
    }
    mSpeedController.stepSpeed();

    if ( mBoard.tileAt(mTank.getCol(),mTank.getRow()) == FLAG ) {
        // if we don't have a window then we're headless (i.e. testing); don't show message boxes for the headless case
        if ( mWindow ) {
            QMessageBox msgBox;
            msgBox.setWindowTitle( "Level completed!");
            msgBox.setText( QString("%1 total moves").arg( mTank.getRecorder().getCount() ) );
            QPushButton* replayButton = msgBox.addButton( QString("&Replay"    ), QMessageBox::ActionRole );
            QPushButton* nextButton   = msgBox.addButton( QString("&Next Level"), QMessageBox::AcceptRole );
            QPushButton* exitButton   = msgBox.addButton( QString("E&xit"      ), QMessageBox::DestructiveRole );
            msgBox.setDefaultButton( nextButton );

            msgBox.exec();

            if ( msgBox.clickedButton() == exitButton ) {
                mWindow->close();
            } else if ( msgBox.clickedButton() == replayButton ) {
                restartLevel( true );
            } else {
                int nextLevel = mBoard.getLevel() + 1;
                if ( nextLevel <= BOARD_MAX_LEVEL ) {
                    mBoard.load( nextLevel );
                }
            }
        }
        return;
    }

    sightCannons();
    mMoveController.wakeup();
}

void Game::onBoardTileChanged( int col, int row )
{
    if ( mBoard.tileAt( col, row ) == DIRT ) {
        sightCannons();
    }
}

bool Game::canPlaceAt(PieceType what, ModelPoint point, int fromAngle, bool futuristic, Piece **pushPiece )
{
    return canPlaceAt( what, point, fromAngle, getBoard(futuristic), pushPiece );
}

bool Game::canPlaceAt(PieceType what, ModelPoint point, int fromAngle, Board* board, Piece **pushPiece )
{
    switch( board->tileAt( point.mCol, point.mRow ) ) {
    case DIRT:
    case TILE_SUNK:
    {   Piece* hit = board->getPieceManager()->pieceAt( point.mCol, point.mRow );
        if ( hit ) {
            if ( what == TANK && pushPiece ) {
                *pushPiece = hit;
                return canMoveFrom( hit->getType(), fromAngle, &point, board );
            }
            return false;
        }
        return true;
    }
    case FLAG:
        return what == TANK;
    case WATER:
        return what != TANK;
    default:
        ;
    }
    return false;
}

MoveController* Game::getMoveController()
{
    return &mMoveController;
}

PathFinderController* Game::getPathFinderController()
{
    return &mPathFinderController;
}

BoardWindow* Game::getWindow() const
{
    return mWindow;
}

bool getShotReflection( int mirrorAngle, int *shotAngle )
{
    switch( mirrorAngle ) {
    case 0:
        switch( *shotAngle ) {
        case   0: *shotAngle =  90; return true;
        case 270: *shotAngle = 180; return true;
        }
        break;
    case 90:
        switch( *shotAngle ) {
        case 90: *shotAngle = 180; return true;
        case  0: *shotAngle = 270; return true;
        }
        break;
    case 180:
        switch( *shotAngle ) {
        case 180: *shotAngle = 270; return true;
        case  90: *shotAngle =   0; return true;
        }
        break;
    case 270:
        switch( *shotAngle ) {
        case 270: *shotAngle =  0; return true;
        case 180: *shotAngle = 90; return true;
        }
        break;
    }
    return false;
}

/**
 * @brief Helps determine if the given moving piece was in hit by a shot
 * @param offset The distance between the shot end point and the piece
 * @param endOffset Returns the offset of the piece if hit
 * @return true if hit (i.e. the moving piece is in strike range), otherwise false
 */
bool onShootThruMovingPiece( int offset, /*int angle,*/ int *endOffset )
{
    if ( abs(offset) < 24 ) {
        // enable if future tracking support implemented for push
//        int col = mMovingPiece.getEndX()/24;
//        int row = mMovingPiece.getEndY()/24;
//        if ( canMoveFrom( mMovingPiece.getType(), angle, &col, &row, false ) ) {
//            mMovingPiece.extend(angle);
//        }
        *endOffset = offset;
        return true;
    }
    return false;
}

bool canShootThruPush( QPoint& centerOfSquare, int angle, Push& push, QPoint *hitPoint )
{
    if ( push.getType() != NONE ) {
        if ( push.getBounds()->contains(centerOfSquare) ) {
            switch( angle ) {
            case  90:
            case 270:
                if ( hitPoint ) {
                    hitPoint->setX( push.getX().toInt()+24/2 );
                    centerToEntryPoint( angle, hitPoint );
                }
                return false;
            case   0:
            case 180:
                if ( hitPoint ) {
                    hitPoint->setY( push.getY().toInt()+24/2 );
                    centerToEntryPoint( angle, hitPoint );
                }
                return false;
            }
        }
    }
    return true;
}

bool Game::canShootThru( int col, int row, int *angle, FutureChange *change, Shooter* source, QPoint *hitPoint )
{
    bool futuristic = (change != 0);
    Board* board = getBoard(futuristic);

    switch( board->tileAt(col,row) ) {
    case DIRT:
    case TILE_SUNK:
    {   Piece* hitPiece = board->getPieceManager()->pieceAt( col, row );
        if ( hitPiece ) {
            switch( hitPiece->getType() ) {
            case TILE_MIRROR:
                if ( getShotReflection( hitPiece->getAngle(), angle ) ) {
                    return true;
                }
                break;

            case CANNON:
            {   int pieceAngle = hitPiece->getAngle();
                if ( abs( pieceAngle - *angle ) == 180 ) {
                    if ( futuristic ) {
                        mFutureDelta.enable();
                        board = &mFutureBoard;
                    }
                    if ( change ) {
                        change->changeType = PIECE_ERASED;
                        change->point = ModelPoint( col, row );
                        change->u.erase.pieceType = CANNON;
                        change->u.erase.pieceAngle = pieceAngle;
                    }
                    board->getPieceManager()->eraseAt( col, row );

                    if ( hitPoint ) {
                        centerToEntryPoint( *angle, hitPoint );
                    }
                    return false;
                }
            }
                break;

            default:
                ;
            }

            // push it:
            ModelPoint toPoint(  col, row );
            if ( canMoveFrom( hitPiece->getType(), *angle, &toPoint, futuristic ) ) {
                if ( futuristic ) {
                    change->changeType = PIECE_PUSHED;
                    change->point = toPoint;
                    change->u.multiPush.pieceType = hitPiece->getType();
                    change->u.multiPush.pieceAngle = hitPiece->getAngle();
                    change->u.multiPush.direction = *angle;
                    change->u.multiPush.count = 1;
                    onFuturePush( hitPiece, *angle );
                } else {
                    SimplePiece simple( hitPiece );
                    board->getPieceManager()->eraseAt( col, row );
                    mShotPush.start( simple, col, row, toPoint.mCol, toPoint.mRow );
                }
            }
            break;
        }

        if ( isMasterBoard(board) ) {
            QPoint centerOfSquare = modelToViewCenterSquare( col, row );
            if ( !canShootThruPush( centerOfSquare, *angle, mTankPush, hitPoint ) ) {
                return false;
            }
            if ( !canShootThruPush( centerOfSquare, *angle, mShotPush, hitPoint ) ) {
                return false;
            }

            // for the tank, vet that the distance is greater than zero to avoid undesireable self-inflicted wounds:
            if ( source ) {
                if ( (source->getType() != TANK || source->getShot().getDistance())
                   && mTank.getRect().contains(centerOfSquare) ) {
                    switch( *angle ) {
                    case  90:
                    case 270:
                        if ( hitPoint ) {
                            hitPoint->setX( mTank.getViewX().toInt()+24/2 );
                            centerToEntryPoint( *angle, hitPoint );
                        }
                        break;
                    case   0:
                    case 180:
                        if ( hitPoint ) {
                            hitPoint->setY( mTank.getViewY().toInt()+24/2 );
                            centerToEntryPoint( *angle, hitPoint );
                        }
                        break;
                    }
                    source->getShot().setIsKill();
                    return false;
                }
            }
        }
        return true;
    }
    case WATER:
    case FLAG:
        return true;

    case STONE_SLIT:    if ( *angle == 90 || *angle == 270 ) return true; break;
    case STONE_SLIT_90: if ( *angle ==  0 || *angle == 180 ) return true; break;

    case STONE_MIRROR:     if ( getShotReflection(   0, angle ) ) return true; break;
    case STONE_MIRROR__90: if ( getShotReflection(  90, angle ) ) return true; break;
    case STONE_MIRROR_180: if ( getShotReflection( 180, angle ) ) return true; break;
    case STONE_MIRROR_270: if ( getShotReflection( 270, angle ) ) return true; break;

    case WOOD:
        if ( futuristic ) {
            mFutureDelta.enable();
            board = &mFutureBoard;
        }
        board->setTileAt( WOOD_DAMAGED, col, row );
        if ( change ) {
            change->changeType = TILE_CHANGE;
            change->point = ModelPoint( col, row );
            change->u.tileType = WOOD;
        }
        break;
    case WOOD_DAMAGED:
        if ( futuristic ) {
            mFutureDelta.enable();
            board = &mFutureBoard;
        }
        board->setTileAt( DIRT, col, row );
        if ( change ) {
            change->changeType = TILE_CHANGE;
            change->point = ModelPoint( col, row );
            change->u.tileType = WOOD_DAMAGED;
        }
        break;

    default:
        ;
    }

    if ( hitPoint ) {
        centerToEntryPoint( *angle, hitPoint );
    }
    return false;
}

void Game::onTankPushingInto( int col, int row, int fromAngle )
{
    PieceSetManager* pm = mBoard.getPieceManager();
    Piece* what = pm->pieceAt( col, row );
    if ( what ) {
        ModelPoint toPoint( col, row );
        if ( getAdjacentPosition( fromAngle, &toPoint ) ) {
            SimplePiece simple( what );
            pm->eraseAt( col, row );
            mTankPush.start( simple, col, row, toPoint.mCol, toPoint.mRow );
        } else {
            std::cout << "*** no adjacent for angle " << fromAngle << std::endl;
        }
    }
}

AnimationStateAggregator* Game::getMoveAggregate()
{
    return &mMoveAggregate;
}

AnimationStateAggregator* Game::getShotAggregate()
{
    return &mShotAggregate;
}

ShotModel& Game::getCannonShot()
{
    return mActiveCannon.getShot();
}

SpeedController *Game::getSpeedController()
{
    return &mSpeedController;
}

void Game::onFuturePush( Piece* pushPiece, int direction )
{
    mFutureDelta.enable();

    // copy values before pushPiece likely deleted:
    ModelPoint point = *pushPiece;
    PieceType type   = pushPiece->getType();
    int pieceAngle   = pushPiece->getAngle();

    if ( !mFutureBoard.getPieceManager()->erase( pushPiece ) ) {
        std::cout << "*** failed to erase future pushPiece at " << point.mCol << "," << point.mRow << std::endl;
    }

    if ( !getAdjacentPosition( direction, &point ) ) {
        std::cout << "*** failed to get future pushPiece position for " << direction << "/" << point.mCol << "," << point.mRow << std::endl;
        return;
    }
    mFutureBoard.applyPushResult( type, point.mCol, point.mRow, pieceAngle );
}

const PieceSet* Game::getDeltaPieces()
{
    if ( mFutureDelta.enabled() ) {
        return mFutureDelta.getPieceManager()->getPieces();
    }
    return 0;
}

void Game::undoFuturePush( MovePiece* pusher )
{
    ModelPoint point = *pusher;
    if ( getAdjacentPosition( pusher->getAngle(), &point ) ) {
        PieceSetManager* pieceManager = mFutureBoard.getPieceManager();
        Piece* pushee = pieceManager->pieceAt( point.mCol, point.mRow );
        if ( pushee ) {
            pieceManager->erase( pushee );
        } else if ( pusher->getPushPieceType() == TILE && mFutureBoard.tileAt( point.mCol, point.mRow ) == TILE_SUNK ) {
            mFutureBoard.setTileAt( WATER, point.mCol, point.mRow );
        }
        pieceManager->insert( pusher->getPushPieceType(), pusher->getCol(), pusher->getRow(), pusher->getPushPieceAngle() );
    }
}

void Game::undoLastMove()
{
    PieceListManager* moveManager = mMoveController.getMoves();
    switch( moveManager->size() ) {
    case 0: // empty
        return;
    case 1: // allow if not doing this move
        if ( mMoveAggregate.active() ) {
            return;
        }
    }

    Piece* piece = moveManager->getBack();
    if ( piece ) {
        if ( piece->hasPush() ) {
            undoFuturePush( dynamic_cast<MovePiece*>(piece) );
        }
        mMoveController.eraseLastMove();
    }
}

void Game::onTankKilled()
{
    // if we don't have a window then we're headless (i.e. testing); don't show message boxes for the headless case
    if ( mWindow ) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Level lost!");
        QPushButton* restartButton = msgBox.addButton( QString("Re&start"), QMessageBox::AcceptRole      );
        QPushButton* replayButton  = msgBox.addButton( QString("&Replay" ), QMessageBox::ActionRole      );
        QPushButton* exitButton    = msgBox.addButton( QString("E&xit"   ), QMessageBox::DestructiveRole );
        msgBox.setDefaultButton( restartButton );

        msgBox.exec();

        if ( msgBox.clickedButton() == exitButton ) {
            mWindow->close();
        } else {
            restartLevel( msgBox.clickedButton() == replayButton );
        }
    }
}

void Game::restartLevel( bool replay )
{
    mMoveController.setReplay( replay );
    mBoard.load( mBoard.getLevel() );
    if ( replay ) {
        mMoveController.wakeup();
    }
}

#include <iostream>
#include <Qt>
#include <QVariant>
#include <QMessageBox>
#include <QPushButton>

#include "game.h"
#include "gameregistry.h"
#include "movecontroller.h"
#include "speedcontroller.h"
#include "pathfindercontroller.h"
#include "animationstateaggregator.h"
#include "model/tank.h"
#include "model/push.h"
#include "model/level.h"
#include "view/boardwindow.h"
#include "util/renderutils.h"

Game::Game() : mBoardLoaded(0)
{
}

void Game::init( GameRegistry* registry )
{
    registry->getTank().init( registry );

    MoveController& moveController = registry->getMoveController();
    moveController.init( registry );
    QObject::connect( &moveController, &MoveController::pushingInto, this, &Game::onTankPushingInto );
    QObject::connect( &moveController, &MoveController::idle, this, &Game::endMoveDeltaTracking );
    QObject::connect( &registry->getPathFinderController(), &PathFinderController::pathFound, &moveController, &MoveController::onPathFound );

    registry->getActiveCannon().init( registry, CANNON, QColor(255,50,83) );

    registry->getPathFinderController().init(this);

    registry->getMoveAggregate().setObjectName("MoveAggregate");
    registry->getShotAggregate().setObjectName("ShotAggregate");

    registry->getTankPush().init( registry );
    registry->getShotPush().init( registry );
    QObject::connect( &registry->getTankPush(), &Push::stateChanged, &registry->getMoveAggregate(), &AnimationStateAggregator::onStateChanged );
    QObject::connect( &registry->getShotPush(), &Push::stateChanged, &registry->getShotAggregate(), &AnimationStateAggregator::onStateChanged );
    QObject::connect( &registry->getMoveAggregate(), &AnimationStateAggregator::finished, this, &Game::onMoveAggregatorFinished, Qt::QueuedConnection );
    QObject::connect( &registry->getShotAggregate(), &AnimationStateAggregator::finished, this, &Game::sightCannons );

    mFutureDelta.init( &mBoard, &mFutureBoard );

    QObject::connect( &mBoard, &Board::tileChangedAt, this, &Game::onBoardTileChanged );
    QObject::connect( &mBoard, &Board::boardLoading,  this, &Game::onBoardLoading );
    QObject::connect( &mBoard, &Board::boardLoaded,   this, &Game::onBoardLoaded, Qt::QueuedConnection );

    if ( BoardWindow* window = registry->getWindow() ) {
        QObject::connect( window, &BoardWindow::focusChanged, &registry->getMoveController(), &MoveController::setFocus );
        QObject::connect( &registry->getTankPush(), &Push::rectDirty, window, &BoardWindow::renderLater );
        QObject::connect( &registry->getShotPush(), &Push::rectDirty, window, &BoardWindow::renderLater );

        QObject::connect( &registry->getTank(), &Tank::changed, window, &BoardWindow::renderLater );
        QObject::connect( &registry->getMoveController().getFutureShots(), &FutureShotPathManager::dirtyRect, window, &BoardWindow::renderLater );

        QMenu& menu = window->getMenu();
        QObject::connect( &menu, &QMenu::aboutToShow, &registry->getTank(), &Tank::pause  );
        QObject::connect( &menu, &QMenu::aboutToHide, &registry->getTank(), &Tank::resume );

        PieceListManager& moveManager = registry->getMoveController().getMoves();
        QObject::connect( &moveManager, &PieceListManager::appended, window, &BoardWindow::renderSquareLater );
        QObject::connect( &moveManager, &PieceListManager::erased,   window, &BoardWindow::renderSquareLater );
        QObject::connect( &moveManager, &PieceListManager::changed,  window, &BoardWindow::renderSquareLater );

        QObject::connect( &mFutureDelta.getPieceManager(), &PieceSetManager::erasedAt,   window, &BoardWindow::renderSquareLater );
        QObject::connect( &mFutureDelta.getPieceManager(), &PieceSetManager::insertedAt, window, &BoardWindow::renderSquareLater );
        QObject::connect( &mFutureDelta.getPieceManager(), &PieceSetManager::changedAt,  window, &BoardWindow::renderSquareLater );

        window->init( registry );
    }

    mBoard.setParent(this);
}

void Game::onBoardLoading()
{
    mBoardLoaded = false;
}

bool Game::isBoardLoaded()
{
    return mBoardLoaded;
}

void Game::onBoardLoaded()
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        mFutureDelta.enable( false );
        registry->getMoveAggregate().reset();
        registry->getShotAggregate().reset();
        registry->getTank().onBoardLoaded( mBoard.getTankStartPoint() );
        registry->getActiveCannon().reset( NullPoint );
        registry->getSpeedController().setHighSpeed(false);
        registry->getMoveController().onBoardLoaded( &mBoard );
    }
    mBoardLoaded = true;
    emit boardLoaded();
}

void Game::endMoveDeltaTracking()
{
    mFutureDelta.enable( false );
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

bool Game::canMoveFrom( PieceType what, int angle, ModelPoint *point, Board* board, Piece **pushPiece ) {
    return getAdjacentPosition(angle, point) && canPlaceAt( what, *point, angle, board, pushPiece );
}

bool Game::canMoveFrom( PieceType what, int angle, ModelPoint *point, bool futuristic, Piece **pushPiece )
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        if ( what != TANK && point->equals( registry->getTank().getPoint() ) ) {
            return false;
        }
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
        return board->getPieceManager().typeAt( col, row ) == NONE;
    case WATER:
    case FLAG:
        return true;
    default:
        return false;
    }
}

void Game::sightCannons()
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        // fire any cannon
        const PieceSet& pieces = mBoard.getPieceManager().getPieces();
        bool sighted = false;
        int tankCol = registry->getTank().getCol();
        int tankRow = registry->getTank().getRow();
        int fireAngle, fireCol, fireRow;

        for( auto it = pieces.cbegin(); !sighted && it != pieces.cend(); ++it ) {
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
            Shooter& activeCannon = registry->getActiveCannon();
            activeCannon.setViewX( fireCol*24 );
            activeCannon.setViewY( fireRow*24 );
            activeCannon.setViewRotation( fireAngle );
            activeCannon.fire();
        }
    }
}

void Game::onPushed(PieceType type, int col, int row, int pieceAngle )
{
    mBoard.applyPushResult( type, col, row, pieceAngle );
}

void Game::onMoveAggregatorFinished()
{
    if ( !mFutureDelta.getPieceManager().size() ) {
        mFutureDelta.enable( false );
    }
    if ( GameRegistry* registry = getRegistry(this) ) {
        registry->getSpeedController().stepSpeed();
        Tank& tank = registry->getTank();

        if ( mBoard.tileAt(tank.getCol(),tank.getRow()) == FLAG ) {
            // if we don't have a window then we're headless (i.e. testing); don't show message boxes for the headless case
            if ( registry->getWindow() ) {
                // ensure this is off now to remove it from the display:
                registry->getMoveController().setReplay( false );

                QMessageBox msgBox;
                msgBox.setWindowTitle( "Level completed!");
                msgBox.setText( QString("%1 total moves").arg( tank.getRecorder().getCount() ) );
                QPushButton* replayButton = msgBox.addButton( QString("&Auto Replay" ), QMessageBox::ActionRole );
                QPushButton* nextButton   = msgBox.addButton( QString("&Next Level"  ), QMessageBox::AcceptRole );
                QPushButton* exitButton   = msgBox.addButton( QString("E&xit"        ), QMessageBox::DestructiveRole );
                msgBox.setDefaultButton( nextButton );

                msgBox.exec();

                if ( msgBox.clickedButton() == exitButton ) {
                    registry->getWindow()->close();
                } else if ( msgBox.clickedButton() == replayButton ) {
                    restartLevel( true );
                } else if ( int i = registry->getLevelList().nextLevel( mBoard.getLevel() ) ) {
                    mBoard.load( i );
                }
            }
            return;
        }

        sightCannons();
        registry->getMoveController().wakeup();
    }
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
    {   Piece* hit = board->getPieceManager().pieceAt( point.mCol, point.mRow );
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
    {   Piece* hitPiece = board->getPieceManager().pieceAt( col, row );
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
                    board->getPieceManager().eraseAt( col, row );

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
                    board->getPieceManager().eraseAt( col, row );
                    if ( GameRegistry* registry = getRegistry(this) ) {
                        registry->getShotPush().start( simple, col, row, toPoint.mCol, toPoint.mRow );
                    }
                }
            }
            break;
        }

        if ( isMasterBoard(board) ) {
            if ( GameRegistry* registry = getRegistry(this) ) {
                QPoint centerOfSquare = modelToViewCenterSquare( col, row );
                if ( !canShootThruPush( centerOfSquare, *angle, registry->getTankPush(), hitPoint ) ) {
                    return false;
                }
                if ( !canShootThruPush( centerOfSquare, *angle, registry->getShotPush(), hitPoint ) ) {
                    return false;
                }

                // for the tank, vet that the distance is greater than zero to avoid undesireable self-inflicted wounds:
                if ( source ) {
                    Tank& tank = registry->getTank();
                    if ( (source->getType() != TANK || source->getShot().getDistance())
                         && tank.getRect().contains(centerOfSquare) ) {
                        switch( *angle ) {
                        case  90:
                        case 270:
                            if ( hitPoint ) {
                                hitPoint->setX( tank.getViewX().toInt()+24/2 );
                                centerToEntryPoint( *angle, hitPoint );
                            }
                            break;
                        case   0:
                        case 180:
                            if ( hitPoint ) {
                                hitPoint->setY( tank.getViewY().toInt()+24/2 );
                                centerToEntryPoint( *angle, hitPoint );
                            }
                            break;
                        }
                        source->getShot().setIsKill();
                        return false;
                    }
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
    PieceSetManager& pm = mBoard.getPieceManager();
    Piece* what = pm.pieceAt( col, row );
    if ( what ) {
        ModelPoint toPoint( col, row );
        if ( getAdjacentPosition( fromAngle, &toPoint ) ) {
            SimplePiece simple( what );
            pm.eraseAt( col, row );
            if ( GameRegistry* registry = getRegistry(this) ) {
                registry->getTankPush().start( simple, col, row, toPoint.mCol, toPoint.mRow );
            }
        } else {
            std::cout << "*** no adjacent for angle " << fromAngle << std::endl;
        }
    }
}

void Game::onFuturePush( Piece* pushPiece, int direction )
{
    mFutureDelta.enable();

    // copy values before pushPiece likely deleted:
    ModelPoint point = *pushPiece;
    PieceType type   = pushPiece->getType();
    int pieceAngle   = pushPiece->getAngle();

    if ( !mFutureBoard.getPieceManager().erase( pushPiece ) ) {
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
        return &mFutureDelta.getPieceManager().getPieces();
    }
    return 0;
}

void Game::undoFuturePush( MovePiece* pusher )
{
    ModelPoint point = *pusher;
    if ( getAdjacentPosition( pusher->getAngle(), &point ) ) {
        PieceSetManager& pieceManager = mFutureBoard.getPieceManager();
        Piece* pushee = pieceManager.pieceAt( point.mCol, point.mRow );
        if ( pushee ) {
            pieceManager.erase( pushee );
        } else if ( pusher->getPushPieceType() == TILE && mFutureBoard.tileAt( point.mCol, point.mRow ) == TILE_SUNK ) {
            mFutureBoard.setTileAt( WATER, point.mCol, point.mRow );
        }
        pieceManager.insert( pusher->getPushPieceType(), pusher->getCol(), pusher->getRow(), pusher->getPushPieceAngle() );
    }
}

void Game::undoLastMove()
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        PieceListManager& moveManager = registry->getMoveController().getMoves();
        switch( moveManager.size() ) {
        case 0: // empty
            return;
        case 1: // allow if not doing this move
            if ( registry->getMoveAggregate().active() ) {
                return;
            }
        }

        Piece* piece = moveManager.getBack();
        if ( piece ) {
            if ( piece->hasPush() ) {
                undoFuturePush( dynamic_cast<MovePiece*>(piece) );
            }
            registry->getMoveController().eraseLastMove();
        }
    }
}

void Game::onTankKilled()
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        // if we don't have a window then we're headless (i.e. testing); don't show message boxes for the headless case
        if ( registry->getWindow() ) {
            // ensure this is off now to remove it from the display:
            registry->getMoveController().setReplay( false );

            QMessageBox msgBox;
            msgBox.setWindowTitle("Level lost!");
            msgBox.setText( "Restart level?\nTip: Select Auto Replay to choose a restore point." );
            QPushButton* restartButton = msgBox.addButton( QString("Re&start"     ), QMessageBox::AcceptRole      );
            QPushButton* replayButton  = msgBox.addButton( QString("&Auto Replay" ), QMessageBox::ActionRole      );
            QPushButton* exitButton    = msgBox.addButton( QString("E&xit"        ), QMessageBox::DestructiveRole );
            msgBox.setDefaultButton( restartButton );

            msgBox.exec();

            if ( msgBox.clickedButton() == exitButton ) {
                registry->getWindow()->close();
            } else {
                restartLevel( msgBox.clickedButton() == replayButton );
            }
        }
    }
}

void Game::restartLevel( bool replay )
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        registry->getMoveController().setReplay( replay );
    }
    mBoard.reload();
}

void Game::replayLevel()
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        if ( !registry->getTank().getRecorder().isEmpty() ) {
            restartLevel( true );
        }
    }
}

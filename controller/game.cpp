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
#include "model/boardpool.h"
#include "view/boardwindow.h"
#include "view/boardrenderer.h"
#include "view/levelcompleteddialog.h"
#include "util/persist.h"
#include "util/imageutils.h"

Game::Game() : mDesiredLevel(0)
{
}

Game::~Game()
{
}

void Game::init( GameRegistry* registry )
{
    registry->getTank().init( registry );

    MoveController& moveController = registry->getMoveController();
    moveController.init( registry );

    registry->getActiveCannon().init( registry, CANNON, QColor(255,50,83) );

    registry->getPathFinderController().init();

    AnimationStateAggregator& moveAggregate = registry->getMoveAggregate(); moveAggregate.setObjectName("MoveAggregate");
    AnimationStateAggregator& shotAggregate = registry->getShotAggregate(); shotAggregate.setObjectName("ShotAggregate");

    Push& tankPush = registry->getTankPush(); tankPush.init( registry );
    Push& shotPush = registry->getShotPush(); shotPush.init( registry );
    QObject::connect( &tankPush, &Push::stateChanged, &moveAggregate, &AnimationStateAggregator::onStateChanged, Qt::DirectConnection );
    QObject::connect( &shotPush, &Push::stateChanged, &shotAggregate, &AnimationStateAggregator::onStateChanged, Qt::DirectConnection );
    QObject::connect( &moveAggregate, &AnimationStateAggregator::finished, this, &Game::onMoveAggregatorFinished, Qt::QueuedConnection );
    QObject::connect( &shotAggregate, &AnimationStateAggregator::finished, this, &Game::sightCannons, Qt::DirectConnection );

    mFutureDelta.init( &mBoard, &mFutureBoard );
    QObject::connect( &moveController, &MoveController::invalidatePushIdDelineation, &mFutureDelta.getPieceManager(), &PieceSetManager::invalidatePushIdDelineation, Qt::DirectConnection );

    QObject::connect( &mBoard, &Board::tileChangedAt, this, &Game::onBoardTileChanged, Qt::DirectConnection );
    QObject::connect( &mBoard, &Board::boardLoading,  this, &Game::onBoardLoading,     Qt::DirectConnection );
    QObject::connect( &mBoard, &Board::boardLoaded,   this, &Game::onBoardLoaded,      Qt::DirectConnection );
    QObject::connect( &registry->getBoardPool(), &BoardPool::boardLoaded, this, &Game::onPoolLoaded, Qt::QueuedConnection );

    if ( BoardWindow* window = registry->getWindow() ) {
        moveController.connectWindow( window );
        window->connectTo( mFutureDelta.getPieceManager() );
    }

    mBoard.setParent(this);
}

void Game::onBoardLoading( int level )
{
    mDesiredLevel = level;
}

bool Game::isBoardLoaded()
{
    return mDesiredLevel && mDesiredLevel == mBoard.getLevel();
}

void Game::onPoolLoaded( int level )
{
    if ( mDesiredLevel == level ) {
        if ( GameRegistry* registry = getRegistry(this) ) {
            if ( Board* board = registry->getBoardPool().find( level ) ) {
                mBoard.load( board );
            }
        }
    }
}

void Game::onBoardLoaded( int level )
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        mFutureDelta.enable( false );
        registry->getMoveAggregate().reset();
        registry->getShotAggregate().reset();
        registry->getActiveCannon().reset( ModelVector(0,0) );
        registry->getSpeedController().setHighSpeed(false);
        registry->getTank().onBoardLoaded( mBoard );
        registry->getRecorder().onBoardLoaded( level );
        registry->getMoveController().onBoardLoaded( mBoard );

        emit boardLoaded();
    }
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
    if ( getAdjacentPosition( angle, point ) ) {
        if ( what != TANK ) {
            // prevent pushing it onto the tank:
            if ( GameRegistry* registry = getRegistry(this) ) {
                if ( point->equals( registry->getMoveController().getDragFocusVector( futuristic ? MOVE : TANK ) ) ) {
                    return false;
                }
            }
        }

        return canPlaceAt( what, *point, angle, getBoard(futuristic), pushPiece );
    }
    return false;
}

bool Game::canCannonSightThru( Board* board, ModelPoint point )
{
    switch( board->tileAt( point ) ) {
    case DIRT:
    case TILE_SUNK:
        if ( board->getPieceManager().typeAt( point ) != NONE ) {
            return false;
        }

        if ( GameRegistry* registry = getRegistry(this) ) {
            if ( registry->getTankPush().occupies( point )
              || registry->getShotPush().occupies( point ) ) {
                return false;
            }
        }
        return true;

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
                        if ( !canCannonSightThru( &mBoard, ModelPoint( tankCol, row ) ) ) {
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
                        if ( !canCannonSightThru( &mBoard, ModelPoint( col, tankRow ) ) ) {
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

void Game::onPushed(PieceType type, ModelPoint point, int pieceAngle )
{
    mBoard.applyPushResult( type, point, pieceAngle );
}

void Game::onMoveAggregatorFinished()
{
    if ( !mFutureDelta.getPieceManager().size() ) {
        mFutureDelta.enable( false );
    }
    if ( GameRegistry* registry = getRegistry(this) ) {
        registry->getSpeedController().stepSpeed();
        Tank& tank = registry->getTank();

        if ( mBoard.tileAt( tank.getPoint() ) == FLAG ) {
            // if we don't have a window then we're headless (i.e. testing); don't show message boxes for the headless case
            if ( registry->getWindow() ) {
                // ensure this is off now to remove it from the display:
                bool wasReplaying = registry->getMoveController().setReplay( false );

                // persist it if wasn't a replay:
                if ( !wasReplaying ) {
                    registry->getPersist().onLevelUpdated( mBoard.getLevel() );
                }

                LevelCompletedDialog dialog( registry );
                dialog.exec();

                switch( dialog.getClickedCode() ) {
                case LevelCompletedDialog::Exit:
                    registry->getWindow()->close();
                    break;
                case LevelCompletedDialog::Replay:
                    restartLevel( true );
                    break;
                case LevelCompletedDialog::Next:
                    if ( int i = registry->getLevelList().nextLevel( mBoard.getLevel() ) ) {
                        loadMasterBoard( i );
                    }
                    break;
                default:
                    break;
                }
            }
            return;
        }

        sightCannons();
        registry->getMoveController().wakeup();
    }
}

void Game::onBoardTileChanged( ModelPoint point )
{
    if ( mBoard.tileAt( point ) == DIRT ) {
        sightCannons();
    }
}

bool Game::canPlaceAt(PieceType what, ModelPoint point, int fromAngle, bool futuristic, Piece **pushPiece )
{
    return canPlaceAt( what, point, fromAngle, getBoard(futuristic), pushPiece );
}

bool Game::canPushPiece( const Piece* piece, int fromAngle )
{
    switch( piece->getType() ) {
    case TILE:
        return true;
    case TILE_MIRROR:
        return piece->getAngle() != fromAngle && (piece->getAngle() + 270) % 360 != fromAngle;
    case CANNON:
        return (piece->getAngle() + 180) % 360 != fromAngle;
    default:
        return false;
    }
}

bool Game::canPlaceAt(PieceType what, ModelPoint point, int fromAngle, Board* board, Piece **pushPiece )
{
    switch( board->tileAt( point ) ) {
    case DIRT:
    case TILE_SUNK:
    {   Piece* hit = board->getPieceManager().pieceAt( point );
        if ( hit ) {
            if ( fromAngle >= 0 ) {
                if ( canPushPiece( hit, fromAngle ) ) {
                    if ( what == TANK ) {
                        if ( pushPiece ) {
                            *pushPiece = hit;
                        }
                        return canMoveFrom( hit->getType(), fromAngle, &point, board );
                    }
                }
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

bool Game::canShootThru( ModelPoint point, int *angle, FutureChange *change, bool apply, Shooter* source,
                         QPoint *hitPoint )
{
    bool futuristic = (change != 0);
    Board* board = getBoard(futuristic);

    switch( board->tileAt( point ) ) {
    case DIRT:
    case TILE_SUNK:
    {   Piece* hitPiece = board->getPieceManager().pieceAt( point );
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
                    if ( futuristic && apply ) {
                        mFutureDelta.enable();
                        board = &mFutureBoard;
                    }
                    if ( change ) {
                        change->changeType = PIECE_ERASED;
                        change->point = point;
                        change->u.erase.pieceType = CANNON;
                        change->u.erase.pieceAngle = pieceAngle;
                        change->u.erase.previousPushedId = hitPiece->getPreviousPushedId();
                    }
                    if ( apply ) {
                        board->getPieceManager().eraseAt( point );
                    }

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
            ModelPoint toPoint( point );
            if ( canMoveFrom( hitPiece->getType(), *angle, &toPoint, futuristic ) ) {
                if ( futuristic ) {
                    change->changeType = PIECE_PUSHED;
                    change->point = toPoint;
                    change->u.multiPush.pieceType = hitPiece->getType();
                    change->u.multiPush.pieceAngle = hitPiece->getAngle();
                    change->u.multiPush.direction = *angle;
                    change->u.multiPush.count = 1;
                    change->u.multiPush.previousPushedId = hitPiece->getPushedId();
                    if ( apply ) {
                        onFuturePush( hitPiece, *angle );
                    }
                } else if ( apply ) {
                    SimplePiece simple( hitPiece );
                    board->getPieceManager().eraseAt( point );
                    if ( GameRegistry* registry = getRegistry(this) ) {
                        registry->getShotPush().start( simple, point, toPoint );
                    }
                }
            }
            break;
        }

        if ( isMasterBoard(board) ) {
            if ( GameRegistry* registry = getRegistry(this) ) {
                QPoint centerOfSquare = point.toViewCenterSquare();
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
                        if ( apply ) {
                            source->getShot().setIsKill();
                        }
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
        if ( apply ) {
            if ( futuristic ) {
                mFutureDelta.enable();
                board = &mFutureBoard;
            }
            board->setTileAt( WOOD_DAMAGED, point );
        }
        if ( change ) {
            change->changeType = TILE_CHANGE;
            change->point = point;
            change->u.tileType = WOOD;
        }
        break;
    case WOOD_DAMAGED:
        if ( apply ) {
            if ( futuristic ) {
                mFutureDelta.enable();
                board = &mFutureBoard;
            }
            board->setTileAt( DIRT, point );
        }
        if ( change ) {
            change->changeType = TILE_CHANGE;
            change->point = point;
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

void Game::onTankPushingInto( ModelPoint point, int fromAngle )
{
    PieceSetManager& pm = mBoard.getPieceManager();
    Piece* what = pm.pieceAt( point );
    if ( what ) {
        ModelPoint toPoint( point );
        if ( getAdjacentPosition( fromAngle, &toPoint ) ) {
            SimplePiece simple( what );
            pm.eraseAt( point );
            if ( GameRegistry* registry = getRegistry(this) ) {
                registry->getTankPush().start( simple, point, toPoint );
            }
        } else {
            std::cout << "*** no adjacent for angle " << fromAngle << std::endl;
        }
    } else {
        std::cout << "*** onTankPushingInto: piece not found" << std::endl;
    }
}

void Game::onFuturePush( Piece* pushPiece, int direction )
{
    mFutureDelta.enable();

    ModelPoint point = *pushPiece;
    if ( !getAdjacentPosition( direction, &point ) ) {
        std::cout << "*** failed to get future pushPiece position for " << direction << "/" << point.mCol << "," << point.mRow << std::endl;
        return;
    }
    mFutureBoard.applyPushResult( pushPiece->getType(), point, pushPiece->getAngle() );
    if ( !mFutureBoard.getPieceManager().erase( pushPiece ) ) {
        std::cout << "*** failed to erase future pushPiece at " << point.mCol << "," << point.mRow << std::endl;
    }
}

const PieceSet* Game::getDeltaPieces()
{
    if ( mFutureDelta.enabled() ) {
        return &mFutureDelta.getPieceManager().getPieces();
    }
    return 0;
}

void Game::loadMasterBoard( int level )
{
    emit mBoard.boardLoading( level );
    if ( GameRegistry* registry = getRegistry(this) ) {
        if ( Board* board = registry->getBoardPool().getBoard( level ) ) {
            mBoard.load( board );
        }
    }
}

void Game::undoFuturePush( MovePiece* pusher )
{
    mFutureBoard.revertPush( pusher );
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
            msgBox.setText( "Restart level?" );
            msgBox.setInformativeText( "Tip: Select Auto Replay to choose a restore point." );
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
    if ( int level = mBoard.getLevel() ) {
        if ( GameRegistry* registry = getRegistry(this) ) {
            registry->getMoveController().setReplay( replay );
            if ( Board* board = registry->getBoardPool().find( level ) ) {
                mBoard.load( board );
            } else {
                // Not in the pool (i.e. a test board)
                std::cout << "* reload board " << level << " not pooled" << std::endl;
                mBoard.reload();
            }
        }
    }
}

void Game::replayLevel()
{
    restartLevel( true );
}

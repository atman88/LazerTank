#include <iostream>
#include <Qt>
#include <QVariant>
#include <QMessageBox>

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

    if ( window ) {
        window->init( this );

        QObject::connect( &mTankPush, &Push::rectDirty, window, &BoardWindow::renderLater );
        QObject::connect( &mShotPush, &Push::rectDirty, window, &BoardWindow::renderLater );

        QObject::connect( &mTank, &Tank::changed, window, &BoardWindow::renderLater );

        QMenu& menu = window->getMenu();
        QObject::connect( &menu, &QMenu::aboutToShow, &mTank, &Tank::pause  );
        QObject::connect( &menu, &QMenu::aboutToHide, &mTank, &Tank::resume );

        PieceListManager* moveManager = mTank.getMoves();
        QObject::connect( moveManager, &PieceListManager::appended, window, &BoardWindow::renderSquareLater );
        QObject::connect( moveManager, &PieceListManager::erased,   window, &BoardWindow::renderSquareLater );
        QObject::connect( moveManager, &PieceListManager::replaced, window, &BoardWindow::renderSquareLater );

        QObject::connect( mFutureDelta.getPieceManager(), &PieceSetManager::erasedAt,   window, &BoardWindow::renderSquareLater );
        QObject::connect( mFutureDelta.getPieceManager(), &PieceSetManager::insertedAt, window, &BoardWindow::renderSquareLater );
    }

    mTank.init( this );
    QObject::connect( &mTank, &Tank::movingInto, this, &Game::onTankMovingInto );

    mActiveCannon.init( this, CANNON, QColor(255,50,83) );

    mPathFinderController.init(this);
    QObject::connect( &mPathFinderController, &PathFinderController::pathFound, this, &Game::endMoveDeltaTracking );

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

    mBoard.load( 1 );
}

void Game::onBoardLoaded()
{
    mFutureDelta.enable( false );
    mTank.reset( mBoard.getTankStartCol(), mBoard.getTankStartRow() );
    mActiveCannon.reset( NullPoint );
    mSpeedController.setHighSpeed(false);
}

void Game::endMoveDeltaTracking()
{
    mFutureDelta.enable( false );
}

GameHandle Game::getHandle()
{
    return mHandle;
}

Board* Game::getBoard()
{
    return &mBoard;
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

bool Game::canMoveFrom(PieceType what, int angle, int *col, int *row, Board* board, bool *pushResult ) {
    return getAdjacentPosition(angle, col, row) && canPlaceAt( what, *col, *row, angle, board, pushResult );
}

bool Game::canMoveFrom(PieceType what, int angle, int *col, int *row, bool futuristic, bool *pushResult ) {
    if ( getAdjacentPosition(angle, col, row) ) {
        Board* board;
        if ( futuristic && mFutureDelta.enabled() ) {
            board = mFutureDelta.getFutureBoard();
        } else {
            board = &mBoard;
        }
        return canPlaceAt( what, *col, *row, angle, board, pushResult );
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

    if ( mBoard.tileAt(mTank.getCol(),mTank.getRow()) == FLAG ) {
        QMessageBox msgBox;
        msgBox.setText("Level completed!");
        msgBox.exec();

        int nextLevel = mBoard.getLevel() + 1;
        if ( nextLevel <= BOARD_MAX_LEVEL ) {
            mBoard.load( nextLevel );
        }
        return;
    }

    sightCannons();
    mTank.wakeup();
}

void Game::onBoardTileChanged( int col, int row )
{
    if ( mBoard.tileAt( col, row ) == DIRT ) {
        sightCannons();
    }
}

bool Game::canPlaceAtNonFuturistic(PieceType what, int col, int row, int fromAngle, bool *pushResult )
{
    return canPlaceAt( what, col, row, fromAngle, &mBoard, pushResult );
}

bool Game::canPlaceAt(PieceType what, int col, int row, int fromAngle, Board* board, bool *pushResult )
{
    switch( board->tileAt(col,row) ) {
    case DIRT:
    case TILE_SUNK:
    {   PieceType hit = board->getPieceManager()->typeAt( col, row );
        if ( hit != NONE ) {
            if ( what == TANK && pushResult ) {
                *pushResult = true;
                return canMoveFrom( hit, fromAngle, &col, &row, board );
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

PathFinderController* Game::getPathFinderController()
{
    return &mPathFinderController;
}

BoardWindow *Game::getWindow() const
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
                hitPoint->setX( push.getX().toInt()+24/2 );
                centerToEntryPoint( angle, hitPoint );
                return false;
            case   0:
            case 180:
                hitPoint->setY( push.getY().toInt()+24/2 );
                centerToEntryPoint( angle, hitPoint );
                return false;
            }
        }
    }
    return true;
}

bool Game::canShootThru( int col, int row, int *angle, Shooter* source, QPoint *hitPoint )
{
    switch( mBoard.tileAt(col,row) ) {
    case DIRT:
    case TILE_SUNK:
    {   PieceSetManager* pm = mBoard.getPieceManager();
        Piece* what = pm->pieceAt( col, row );
        if ( what ) {
            switch( what->getType() ) {
            case TILE_MIRROR:
                if ( getShotReflection( what->getAngle(), angle ) ) {
                    return true;
                }
                break;
            case CANNON:
                if ( abs( what->getAngle() - *angle ) == 180 ) {
                    pm->eraseAt( col, row );
                    centerToEntryPoint( *angle, hitPoint );
                    return false;
                }
                break;
            default:
                ;
            }

            // push it:
            int toCol = col, toRow = row;
            if ( canMoveFrom( what->getType(), *angle, &toCol, &toRow, false ) ) {
                SimplePiece simple( what );
                pm->eraseAt( col, row );
                mShotPush.start( simple, col*24, row*24, toCol*24, toRow*24 );
            }
            break;
        }

        QPoint centerOfSquare = modelToViewCenterSquare( col, row );
        if ( !canShootThruPush( centerOfSquare, *angle, mTankPush, hitPoint ) ) {
            return false;
        }
        if ( !canShootThruPush( centerOfSquare, *angle, mShotPush, hitPoint ) ) {
            return false;
        }

        // for the tank, vet that the distance is greater than zero to avoid undesireable self-inflicted wounds:
        if ( (source->getType() != TANK || source->getShot().getDistance()) && mTank.getRect().contains(centerOfSquare) ) {
            switch( *angle ) {
            case  90:
            case 270:
                hitPoint->setX( mTank.getViewX().toInt()+24/2 );
                centerToEntryPoint( *angle, hitPoint );
                break;
            case   0:
            case 180:
                hitPoint->setY( mTank.getViewY().toInt()+24/2 );
                centerToEntryPoint( *angle, hitPoint );
                break;
            }
            source->getShot().setIsKill();
            return false;
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

    case WOOD:         mBoard.setTileAt( WOOD_DAMAGED, col, row ); break;
    case WOOD_DAMAGED: mBoard.setTileAt( DIRT,         col, row ); break;

    default:
        ;
    }

    centerToEntryPoint( *angle, hitPoint );
    return false;
}

void Game::onTankMovingInto( int col, int row, int fromAngle )
{
    PieceSetManager* pm = mBoard.getPieceManager();
    Piece* what = pm->pieceAt( col, row );
    if ( what ) {
        int toCol = col, toRow = row;
        if ( getAdjacentPosition(fromAngle, &toCol, &toRow) ) {
            SimplePiece simple( what );
            pm->eraseAt( col, row );
            mTankPush.start( simple, col*24, row*24, toCol*24, toRow*24 );
        } else {
            std::cout << "no adjacent for angle " << fromAngle << std::endl;
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

void Game::onFuturePush( int col, int row, int direction )
{
    mFutureDelta.enable();

    PieceType pushedType;
    int pieceAngle;

    // scoping pushedPiece here so it falls out of scope after erased:
    {   Piece* pushedPiece = mFutureBoard.getPieceManager()->pieceAt( col, row );
        if ( !pushedPiece ) {
            std::cout << "*** pushed piece not found!" << std::endl;
            return;
        }
        pushedType = pushedPiece->getType();
        pieceAngle = pushedPiece->getAngle();
        if ( !mFutureBoard.getPieceManager()->erase( pushedPiece ) ) {
            std::cout << "*** failed to erase future piece at " << col << "," << row << std::endl;
        }
    }

    if ( !getAdjacentPosition( direction, &col, &row ) ) {
        std::cout << "*** failed to get future push position for " << direction << "/" << col << "," << row << std::endl;
        return;
    }
    mFutureBoard.applyPushResult( pushedType, col, row, pieceAngle );
}

const PieceSet* Game::getDeltaPieces()
{
    if ( mFutureDelta.enabled() ) {
        return mFutureDelta.getPieceManager()->getPieces();
    }
    return 0;
}

void Game::undoFuturePush( Piece* pusher )
{
    int col = pusher->getCol();
    int row = pusher->getRow();
    if ( getAdjacentPosition( pusher->getAngle(), &col, &row ) ) {
        Piece* pushee = mFutureBoard.getPieceManager()->pieceAt( col, row );
        if ( pushee ) {
            PieceType type = pushee->getType();
            int angle = pushee->getAngle();
            PieceSetManager* pieceManager = mFutureBoard.getPieceManager();
            pieceManager->erase( pushee );
            pieceManager->insert( type, pusher->getCol(), pusher->getRow(), angle );
        }
    }
}

void Game::undoLastMove()
{
    PieceListManager* moveManager = getTank()->getMoves();
    switch( moveManager->size() ) {
    case 0: // empty
        return;
    case 1: // allow if not doing this move
        if ( mMoveAggregate.active() ) {
            return;
        }
    }

    Piece* piece = moveManager->getList()->back();
    if ( piece ) {
        if ( piece->hasPush() ) {
            undoFuturePush( piece );
        }
        getTank()->getMoves()->eraseBack();
    }
}

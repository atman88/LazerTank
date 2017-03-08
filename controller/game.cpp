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
    window->init( this );

    mMovingPiece.setParent( this );
    QObject::connect( &mMovingPiece, &Push::rectDirty, window, &BoardWindow::renderLater );

    mTank.init( this );
    QObject::connect( &mTank, &Tank::movingInto, this, &Game::onTankMovingInto );
    QObject::connect( &mTank, &Tank::moved,      this, &Game::onTankMoved      );
    QObject::connect( &mTank, &Tank::changed, window, &BoardWindow::renderLater );

    QMenu& menu = window->getMenu();
    QObject::connect( &menu, &QMenu::aboutToShow, &mTank, &Tank::pause  );
    QObject::connect( &menu, &QMenu::aboutToHide, &mTank, &Tank::resume );

    mActiveCannon.setParent(this);
    mActiveCannon.init( this, QColor(255,50,83) );

    PieceListManager* moveManager = mTank.getMoves();
    QObject::connect( moveManager, &PieceListManager::appended, window, &BoardWindow::renderSquareLater );
    QObject::connect( moveManager, &PieceListManager::erased,   window, &BoardWindow::renderSquareLater );
    QObject::connect( moveManager, &PieceListManager::replaced, window, &BoardWindow::renderSquareLater );

    mPathFinderController.init(this);
    QObject::connect( &mPathFinderController, &PathFinderController::pathFound, this, &Game::endMoveDeltaTracking );

    mMoveAggregate.setObjectName("MoveAggregate");
    mShotAggregate.setObjectName("ShotAggregate");

    mMovingPiece.init( this );
    mFutureDelta.init( &mBoard, &mFutureBoard );

    QObject::connect( &mBoard, &Board::tileChangedAt, this, &Game::onBoardTileChanged );
    QObject::connect( &mBoard, &Board::boardLoaded,   this, &Game::onBoardLoaded      );

    QObject::connect( mFutureDelta.getPieceManager(), &PieceSetManager::erasedAt,   window, &BoardWindow::renderSquareLater );
    QObject::connect( mFutureDelta.getPieceManager(), &PieceSetManager::insertedAt, window, &BoardWindow::renderSquareLater );

    QObject::connect( &mMovingPiece, &Push::stateChanged, this, &Game::onMovingPieceChanged );

    QObject::connect( &mTank, &Tank::idled, this, &Game::endMoveDeltaTracking );

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

Push& Game::getMovingPiece()
{
    return mMovingPiece;
}

/**
 * @brief Helper method to determine the neighbor square for the given direction
 * @param angle The direction. Legal values are 0, 90, 180, 270.
 * @param col Input starting column. Returns the resultant column
 * @param row Input starting row. Returns the resultant row
 * @return true if the angle is legal
 */
bool getAdjacentPosition( int angle, int *col, int *row )
{
    switch( angle ) {
    case   0: *row -= 1; return true;
    case  90: *col += 1; return true;
    case 180: *row += 1; return true;
    case 270: *col -= 1; return true;
    default:
        ;
    }
    return false;
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


bool Game::advanceShot(int *angle, int *col, int *row, int *endOffset, ShotModel* source ) {
    return getAdjacentPosition(*angle, col, row) && canShootThru( *col, *row, angle, endOffset, source );
}

void Game::onTankMoved( int col, int row )
{
    if ( mBoard.tileAt(col,row) == FLAG ) {
        QMessageBox msgBox;
        msgBox.setText("Level completed!");
        msgBox.exec();

        int nextLevel = mBoard.getLevel() + 1;
        if ( nextLevel <= BOARD_MAX_LEVEL ) {
            mBoard.load( nextLevel );
        }
    }

    if ( mMovingPiece.getType() == NONE ) {
        sightCannons();
    }
}

bool canSightThru( Board* board, int col, int row )
{
    switch( board->tileAt( col, row ) ) {
    case DIRT:
    case TILE_SUNK:
        return board->getPieceManager()->typeAt( col, row ) == NONE;
    case WATER:
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
        mActiveCannon.setX( fireCol*24 );
        mActiveCannon.setY( fireRow*24 );
        mActiveCannon.setRotation( fireAngle );
        mActiveCannon.fire();
    }
}

void Game::onMovingPieceChanged(QAbstractAnimation::State newState, QAbstractAnimation::State oldState  __attribute__((unused)) )
{
    if ( newState == QAbstractAnimation::Stopped ) {
        sightCannons();
    }
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

bool Game::canShootThru( int col, int row, int *angle, int *endOffset, ShotModel* source )
{
    *endOffset = 0;

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
                mMovingPiece.start( simple, col*24, row*24, toCol*24, toRow*24 );
            }
            break;
        }

        if ( mMovingPiece.getType() != NONE ) {
            int movingPieceX = mMovingPiece.getX().toInt();
            int movingPieceY = mMovingPiece.getY().toInt();
            switch( *angle ) {
            case  90:
            case 270:
                if ( row == movingPieceY/24 && onShootThruMovingPiece( movingPieceX - col*24, endOffset ) ) {
                    return false;
                }
                break;
            case   0:
            case 180:
                if ( col == movingPieceX/24 && onShootThruMovingPiece( movingPieceY - row*24, endOffset ) ) {
                    return false;
                }
                break;
            }
        }

        if ( col == mTank.getCol() && row == mTank.getRow() ) {
            source->setIsKill();
            return false;
        }

        return true;
    }
    case WATER:
    case FLAG:
        return true;
    case STONE_SLIT:
        return *angle == 90 || *angle == 270;
    case STONE_SLIT_90:
        return *angle == 0 || *angle == 180;
    case STONE_MIRROR:     return getShotReflection(   0, angle );
    case STONE_MIRROR__90: return getShotReflection(  90, angle );
    case STONE_MIRROR_180: return getShotReflection( 180, angle );
    case STONE_MIRROR_270: return getShotReflection( 270, angle );
    case WOOD:
        mBoard.setTileAt( WOOD_DAMAGED, col, row );
        break;
    case WOOD_DAMAGED:
        mBoard.setTileAt( DIRT, col, row );
        break;
    default:
        ;
    }
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
            mMovingPiece.start( simple, col*24, row*24, toCol*24, toRow*24 );
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

void Game::onFuturePush( Piece* pushingPiece )
{
    mFutureDelta.enable();

    PieceType pushedType;
    int col = pushingPiece->getCol();
    int row = pushingPiece->getRow();
    int angle = pushingPiece->getAngle();

    // scoping pushedPiece here so it falls out of scope after erased:
    {   Piece* pushedPiece = mFutureBoard.getPieceManager()->pieceAt( col, row );
        if ( !pushedPiece ) {
            std::cout << "*** pushed piece not found!" << std::endl;
            return;
        }
        pushedType = pushedPiece->getType();
        if ( !mFutureBoard.getPieceManager()->erase( pushedPiece ) ) {
            std::cout << "*** failed to erase future piece at " << col << "," << row << std::endl;
        }
    }

    if ( !getAdjacentPosition( angle, &col, &row ) ) {
        std::cout << "*** failed to get future push position for " << angle << "/" << col << "," << row << std::endl;
        return;
    }
    mFutureBoard.applyPushResult( pushedType, col, row, angle );
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

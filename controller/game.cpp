#include <iostream>
#include <Qt>
#include <QVariant>
#include <QMessageBox>

#include "game.h"
#include "speedcontroller.h"

Game::Game() : mTankBoardX(0), mTankBoardY(0)
{
    mHandle.game = this;
    setProperty("GameHandle", QVariant::fromValue(mHandle));
}

void Game::init( BoardWindow* window )
{
    window->setGame( mHandle );
    mMovingPiece.setParent( this );

    mActiveCannon.setParent(this);
    mPathFinder.setParent(this);
    mMoveAggregate.setObjectName("MoveAggregate");
    mShotAggregate.setObjectName("ShotAggregate");

    mMovingPiece.init( this );
    mCannonShot.setColor( QColor(255,50,83) );
    mFutureDelta.init( &mBoard, &mFutureBoard );

    QObject::connect( &mBoard, &Board::tileChangedAt, this, &Game::onBoardTileChanged );
    QObject::connect( &mBoard, &Board::boardLoaded,   this, &Game::onBoardLoaded      );

    QObject::connect( &mCannonShot, &ShotModel::rectDirty, window, &BoardWindow::renderLater );
    QObject::connect( mFutureDelta.getPieceManager(), &PieceSetManager::erasedAt,   window, &BoardWindow::renderSquareLater );
    QObject::connect( mFutureDelta.getPieceManager(), &PieceSetManager::insertedAt, window, &BoardWindow::renderSquareLater );

    QObject::connect( &mMovingPiece, &Push::stateChanged, this, &Game::onMovingPieceChanged );

    QObject::connect( window, &BoardWindow::setSpeed, &mSpeedController, &SpeedController::setSpeed );

    QObject::connect( window->getTank(), &Tank::idled,           this, &Game::endMoveDeltaTracking );
    QObject::connect( &mPathFinder,      &PathFinder::pathFound, this, &Game::endMoveDeltaTracking );

    QObject::connect( &mPathFinder, &PathFinder::pathFound, window->getTank()->getMoves(), &PieceListManager::reset );

    QObject::connect( &mCannonShot, &ShotModel::tankKilled, window, &BoardWindow::onTankKilled );

    mBoard.load( 1 );
}

void Game::onBoardLoaded()
{
    mTankBoardX = mBoard.getTankWayPointX();
    mTankBoardY = mBoard.getTankWayPointY();
    mFutureDelta.enable( false );
    mCannonShot.reset();
    mSpeedController.setSpeed(LOW_SPEED);
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

Push& Game::getMovingPiece()
{
    return mMovingPiece;
}

/**
 * @brief Helper method to determine the neighbor square for the given direction
 * @param angle The direction. Legal values are 0, 90, 180, 270.
 * @param x Input starting column. Returns the resultant column
 * @param y Input starting row. Returns the resultant row
 * @return true if the angle is legal
 */
bool getAdjacentPosition( int angle, int *x, int *y )
{
    switch( angle ) {
    case   0: *y -= 1; return true;
    case  90: *x += 1; return true;
    case 180: *y += 1; return true;
    case 270: *x -= 1; return true;
    default:
        ;
    }
    return false;
}

bool Game::canMoveFrom(PieceType what, int angle, int *x, int *y, Board* board, bool *pushResult ) {
    return getAdjacentPosition(angle, x, y) && canPlaceAt( what, *x, *y, angle, board, pushResult );
}

bool Game::canMoveFrom(PieceType what, int angle, int *x, int *y, bool futuristic, bool *pushResult ) {
    if ( getAdjacentPosition(angle, x, y) ) {
        Board* board;
        if ( futuristic && mFutureDelta.enabled() ) {
            board = mFutureDelta.getFutureBoard();
        } else {
            board = &mBoard;
        }
        return canPlaceAt( what, *x, *y, angle, board, pushResult );
    }
    return false;
}


bool Game::advanceShot(int *angle, int *x, int *y, int *endOffset, ShotModel* source ) {
    return getAdjacentPosition(*angle, x, y) && canShootThru( *x, *y, angle, endOffset, source );
}

void Game::onTankMoved( int x, int y )
{
    mTankBoardX = x;
    mTankBoardY = y;

    if ( mBoard.tileAt(x,y) == FLAG ) {
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

bool canSightThru( Board* board, int x, int y )
{
    switch( board->tileAt( x, y ) ) {
    case DIRT:
    case TILE_SUNK:
        return board->getPieceManager()->typeAt( x, y ) == NONE;
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
    int fireAngle, fireX, fireY;
    for( auto it = pieces->cbegin(); !sighted && it != pieces->cend(); ++it ) {
        if ( (*it)->getType() == CANNON ) {
            fireAngle = (*it)->getAngle();
            if ( mTankBoardX == (*it)->getX() ) {
                fireY = (*it)->getY();
                int dir;
                if ( fireAngle == 0 && mTankBoardY < fireY ) {
                    dir = -1;
                } else if ( fireAngle == 180 && mTankBoardY > fireY ) {
                    dir = 1;
                } else {
                    continue;
                }
                for( int y = fireY+dir; ; y += dir ) {
                    if ( y == mTankBoardY ) {
                        fireX = mTankBoardX;
                        sighted = true;
                        break;
                    }
                    if ( !canSightThru( &mBoard, mTankBoardX, y ) ) {
                        break;
                    }
                }
            } else if ( mTankBoardY == (*it)->getY() ) {
                fireX = (*it)->getX();
                int dir;
                if ( fireAngle == 270 && mTankBoardX < fireX ) {
                    dir = -1;
                } else if ( fireAngle == 90 && mTankBoardX > fireX ) {
                    dir = 1;
                } else {
                    continue;
                }
                for( int x = fireX+dir; ; x += dir ) {
                    if ( x == mTankBoardX ) {
                        fireY = mTankBoardY;
                        sighted = true;
                        break;
                    }
                    if ( !canSightThru( &mBoard, x, mTankBoardY ) ) {
                        break;
                    }
                }
            }
        }
    }
    if ( sighted ) {
        mCannonShot.setParent( &mActiveCannon );
        mActiveCannon.setX( fireX*24 );
        mActiveCannon.setY( fireY*24 );
        mActiveCannon.setRotation( fireAngle );
        mCannonShot.fire( &mActiveCannon );
    }
}

void Game::onMovingPieceChanged(QAbstractAnimation::State newState, QAbstractAnimation::State oldState  __attribute__((unused)) )
{
    if ( newState == QAbstractAnimation::Stopped ) {
        sightCannons();
    }
}

void Game::onBoardTileChanged( int x, int y )
{
    if ( mBoard.tileAt( x, y ) == DIRT ) {
        sightCannons();
    }
}

bool Game::canPlaceAtNonFuturistic(PieceType what, int x, int y, int fromAngle, bool *pushResult )
{
    return canPlaceAt( what, x, y, fromAngle, &mBoard, pushResult );
}

bool Game::canPlaceAt(PieceType what, int x, int y, int fromAngle, Board* board, bool *pushResult )
{
    switch( board->tileAt(x,y) ) {
    case DIRT:
    case TILE_SUNK:
    {   PieceType hit = board->getPieceManager()->typeAt( x, y );
        if ( hit != NONE ) {
            if ( what == TANK && pushResult ) {
                *pushResult = true;
                return canMoveFrom( hit, fromAngle, &x, &y, board );
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
//        int x = mMovingPiece.getEndX()/24;
//        int y = mMovingPiece.getEndY()/24;
//        if ( canMoveFrom( mMovingPiece.getType(), angle, &x, &y, false ) ) {
//            mMovingPiece.extend(angle);
//        }
        *endOffset = offset;
        return true;
    }
    return false;
}

bool Game::canShootThru( int x, int y, int *angle, int *endOffset, ShotModel* source )
{
    *endOffset = 0;

    switch( mBoard.tileAt(x,y) ) {
    case DIRT:
    case TILE_SUNK:
    {   PieceSetManager* pm = mBoard.getPieceManager();
        Piece* what = pm->pieceAt( x, y );
        if ( what ) {
            switch( what->getType() ) {
            case TILE_MIRROR:
                if ( getShotReflection( what->getAngle(), angle ) ) {
                    return true;
                }
                break;
            case CANNON:
                if ( abs( what->getAngle() - *angle ) == 180 ) {
                    pm->eraseAt( x, y );
                    return false;
                }
                break;
            default:
                ;
            }

            // push it:
            int toX = x, toY = y;
            if ( canMoveFrom( what->getType(), *angle, &toX, &toY, false ) ) {
                SimplePiece simple( what );
                pm->eraseAt( x, y );
                mMovingPiece.start( simple, x*24, y*24, toX*24, toY*24 );
            }
            break;
        }

        if ( mMovingPiece.getType() != NONE ) {
            int movingPieceX = mMovingPiece.getX().toInt();
            int movingPieceY = mMovingPiece.getY().toInt();
            switch( *angle ) {
            case  90:
            case 270:
                if ( y == movingPieceY/24 && onShootThruMovingPiece( movingPieceX - x*24, endOffset ) ) {
                    return false;
                }
                break;
            case   0:
            case 180:
                if ( x == movingPieceX/24 && onShootThruMovingPiece( movingPieceY - y*24, endOffset ) ) {
                    return false;
                }
                break;
            }
        }

        if ( x == mTankBoardX && y == mTankBoardY ) {
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
        mBoard.setTileAt( WOOD_DAMAGED, x, y );
        break;
    case WOOD_DAMAGED:
        mBoard.setTileAt( DIRT, x, y );
        break;
    default:
        ;
    }
    return false;
}

void Game::onTankMovingInto( int x, int y, int fromAngle )
{
    PieceSetManager* pm = mBoard.getPieceManager();
    Piece* what = pm->pieceAt( x, y );
    if ( what ) {
        int toX = x, toY = y;
        if ( getAdjacentPosition(fromAngle, &toX, &toY) ) {
            SimplePiece simple( what );
            pm->eraseAt( x, y );
            mMovingPiece.start( simple, x*24, y*24, toX*24, toY*24 );
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
    return mCannonShot;
}

SpeedController *Game::getSpeedController()
{
    return &mSpeedController;
}

void Game::onFuturePush( Piece* pushingPiece )
{
    mFutureDelta.enable();

    PieceType pushedType;
    int x = pushingPiece->getX();
    int y = pushingPiece->getY();
    int angle = pushingPiece->getAngle();

    // scoping pushedPiece here so it falls out of scope after erased:
    {   Piece* pushedPiece = mFutureBoard.getPieceManager()->pieceAt( x, y );
        if ( !pushedPiece ) {
            std::cout << "*** pushed piece not found!" << std::endl;
            return;
        }
        pushedType = pushedPiece->getType();
        if ( !mFutureBoard.getPieceManager()->erase( pushedPiece ) ) {
            std::cout << "*** failed to erase future piece at " << x << "," << y << std::endl;
        }
    }

    if ( !getAdjacentPosition( angle, &x, &y ) ) {
        std::cout << "*** failed to get future push position for " << angle << "/" << x << "," << y << std::endl;
        return;
    }
    mFutureBoard.applyPushResult( pushedType, x, y, angle );
}

void Game::findPath(int targetX, int targetY , int startingDirection )
{
    // the path finder doesn't support pushing, so cancel any future moves before using:
    mFutureDelta.enable( false );

    mPathFinder.findPath( targetX, targetY, mTankBoardX, mTankBoardY, startingDirection );
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
    int x = pusher->getX();
    int y = pusher->getY();
    if ( getAdjacentPosition( pusher->getAngle(), &x, &y ) ) {
        Piece* pushee = mFutureBoard.getPieceManager()->pieceAt( x, y );
        if ( pushee ) {
            PieceType type = pushee->getType();
            int angle = pushee->getAngle();
            PieceSetManager* pieceManager = mFutureBoard.getPieceManager();
            pieceManager->erase( pushee );
            pieceManager->insert( type, pusher->getX(), pusher->getY(), angle );
        }
    }
}

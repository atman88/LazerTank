#include <iostream>
#include <QVariant>
#include <QMessageBox>

#include "controller/Game.h"

Game::Game( Board* board )
{
    mBoard = board;
    if ( board ) {
        QObject::connect( board, &Board::tileChangedAt, this, &Game::onBoardTileChanged);
    }
    mHandle.game = this;
    setProperty("GameHandle", QVariant::fromValue(mHandle));
    mActiveCannon.setParent(this);
    mMoveAggregate.setObjectName("MoveAggregate");
    mShotAggregate.setObjectName("ShotAggregate");
    mMovingPiece.setParent( this );
    mMovingPiece.init( this );
}

void Game::init( BoardWindow* window )
{
    QObject::connect( &mCannonShot, &Shot::pathAdded,     window, &BoardWindow::renderSquareLater );
    QObject::connect( &mCannonShot, &Shot::pathRemoved,   window, &BoardWindow::renderSquareLater );
    QObject::connect( &mMovingPiece, &Push::stateChanged, this,   &Game::onMovingPieceChanged     );
}

GameHandle Game::getHandle()
{
    return mHandle;
}

Board* Game::getBoard()
{
    return mBoard;
}

Push& Game::getMovingPiece()
{
    return mMovingPiece;
}

bool Game::canMoveFrom( PieceType what, int angle, int *x, int *y, bool canPush ) {
    return getAdjacentPosition(angle, x, y) && canPlaceAt( what, *x, *y, angle, canPush );
}

bool Game::canShootFrom(int *angle, int *x, int *y ) {
    return getAdjacentPosition(*angle, x, y) && canShootThru( *x, *y, angle );
}

void Game::onTankMoved( int x, int y )
{
//    cout << "moved to " << x << "," << y << std::endl;
    if ( mBoard && mBoard->tileAt(x,y) == FLAG ) {
        QMessageBox msgBox;
        msgBox.setText("Level completed!");
        msgBox.exec();

        int nextLevel = mBoard->getLevel() + 1;
        if ( nextLevel <= BOARD_MAX_LEVEL ) {
            mBoard->load( nextLevel );
        }
    }

    mTankBoardX = x;
    mTankBoardY = y;

    if ( mMovingPiece.getType() == NONE ) {
        sightCannons();
    }
}

void Game::sightCannons()
{
    // fire any cannon
    const PieceSet& pieces = mBoard->getPieces();
    bool sighted = false;
    int fireAngle, fireX, fireY;
    for( auto it = pieces.cbegin(); !sighted && it != pieces.cend(); ++it ) {
        if ( it->getType() == CANNON ) {
            fireAngle = it->getAngle();
            if ( mTankBoardX == it->getX() ) {
                fireY = it->getY();
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
                    if ( !mBoard->canSightThru( mTankBoardX, y ) ) {
                        break;
                    }
                }
            } else if ( mTankBoardY == it->getY() ) {
                fireX = it->getX();
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
                    if ( !mBoard->canSightThru( x, mTankBoardY ) ) {
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
        mCannonShot.fire( fireAngle );
    }
}

void Game::onMovingPieceChanged(QAbstractAnimation::State newState, QAbstractAnimation::State oldState  __attribute__((unused)) )
{
    if ( newState == QAbstractAnimation::Stopped ) {
        sightCannons();
    }
}

bool Game::getAdjacentPosition( int angle, int *x, int *y )
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

void Game::onBoardTileChanged( int x, int y )
{
    if ( mBoard->tileAt( x, y ) == DIRT ) {
        sightCannons();
    }
}

bool Game::canPlaceAt(PieceType what, int x, int y, int fromAngle, bool canPush )
{
    switch( mBoard->tileAt(x,y) ) {
    case DIRT:
    case TILE_SUNK:
    {   PieceType what = mBoard->pieceTypeAt( x, y );
        if ( what != NONE ) {
            return canPush && canMoveFrom( what, fromAngle, &x, &y, false );
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

bool Game::canShootThru( int x, int y, int *angle )
{
    switch( mBoard->tileAt(x,y) ) {
    case DIRT:
    case TILE_SUNK:
    {   Piece what;
        if ( mBoard->pieceAt( x, y, &what ) ) {
            switch( what.getType() ) {
            case TILE_MIRROR:
                if ( getShotReflection( what.getAngle(), angle ) ) {
                    return true;
                }
                break;
            case CANNON:
                if ( abs( what.getAngle() - *angle ) == 180 ) {
                    mBoard->erasePieceAt( x, y );
                    return false;
                }
                break;
            default:
                ;
            }

            // push it:
            int toX = x, toY = y;
            if ( canMoveFrom( what.getType(), *angle, &toX, &toY, false ) ) {
                mBoard->erasePieceAt( x, y );
                mMovingPiece.start( what, x*24, y*24, toX*24, toY*24 );
            }
            break;
        }
        if ( x == mTankBoardX && y == mTankBoardY ) {
            QMessageBox msgBox;
            msgBox.setText("Level lost!");
            msgBox.exec();
            mBoard->load( mBoard->getLevel() );
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
        mBoard->setTileAt( WOOD_DAMAGED, x, y );
        break;
    case WOOD_DAMAGED:
        mBoard->setTileAt( DIRT, x, y );
        break;
    default:
        ;
    }
    return false;
}

void Game::onTankMovingInto( int x, int y, int fromAngle )
{
    if ( mBoard ) {
        Piece what;
        if ( mBoard->pieceAt( x, y, &what ) ) {
            int toX = x, toY = y;
            if ( getAdjacentPosition(fromAngle, &toX, &toY) ) {
                mBoard->erasePieceAt( x, y );
                mMovingPiece.start( what, x*24, y*24, toX*24, toY*24 );
            } else {
                cout << "no adjacent for angle " << fromAngle << std::endl;
            }
        }
    }
}

AnimationAggregator* Game::getMoveAggregate()
{
    return &mMoveAggregate;
}

AnimationAggregator* Game::getShotAggregate()
{
    return &mShotAggregate;
}

Shot& Game::getCannonShot()
{
    return mCannonShot;
}

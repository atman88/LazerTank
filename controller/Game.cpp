#include <iostream>
#include <QVariant>
#include <QMessageBox>

#include "controller/Game.h"

Game::Game( Board* board )
{
    mBoard = board;
    if ( board ) {
        QObject::connect( board, &Board::tileChanged, this, &Game::onBoardTileChanged);
    }
    mHandle.game = this;
    setProperty("GameHandle", QVariant::fromValue(mHandle));
    mMovingPiece.setParent( this );
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
    return getAdjacentPosition(angle, x, y) && canPlaceAt( what, *x, *y );
}

bool Game::canShootFrom( int angle, int *x, int *y ) {
    return getAdjacentPosition(angle, x, y) && canShootThru( angle, *x, *y );
}

void Game::onTankMoved( int x, int y )
{
    cout << "moved to " << x << "," << y << std::endl;
    if ( mBoard && mBoard->tileAt(x,y) == FLAG ) {
        QMessageBox msgBox;
        msgBox.setText("Level completed!");
        msgBox.exec();

        int nextLevel = mBoard->getLevel() + 1;
        if ( nextLevel <= BOARD_MAX_LEVEL ) {
            mBoard->load( nextLevel );
        }
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
    emit rectDirty( QRect( x*24, y*24, 24, 24 ) );
}

bool Game::canPlaceAt( PieceType what, int x, int y )
{
    switch( mBoard->tileAt(x,y) ) {
    case DIRT:
    case TILE_SUNK:
        return true;
    case FLAG:
        return what == TANK;
    case WATER:
        return what != TANK;
    default:
        ;
    }
    return false;
}

bool Game::canShootThru( int angle, int x, int y )
{
    switch( mBoard->tileAt(x,y) ) {
    case DIRT:
    case TILE_SUNK:
    {   PieceType type = mBoard->pieceAt(x,y);
        if ( type != NONE ) {
            int toX = x, toY = y;
            if ( canMoveFrom( type, angle, &toX, &toY, false ) ) {
                mBoard->erasePieceAt( x, y );
                mMovingPiece.start( type, x*24, y*24, toX*24, toY*24 );
            }
            break;
        }
        return true;
    }
    case WATER:
        return true;
    default:
        ;
    }
    return false;
}

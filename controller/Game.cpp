#include <QVariant>
#include "controller/Game.h"

Game::Game( Board* board )
{
    mBoard = board;
    if ( board ) {
        mTankX = board->mInitialTankX;
        mTankY = board->mInitialTankY;
        if ( mTankX >= 0 && mTankY >= 0 ) {
            emit tankInitialized( mTankX, mTankY );
        }
    }
    mHandle.game = this;

    setProperty("IntentList", QVariant::fromValue(mIntents));
}

GameHandle Game::getHandle()
{
    return mHandle;
}

Board* Game::getBoard()
{
    return mBoard;
}

bool Game::canMoveFrom( int angle, int *x, int *y ) {
    return getAdjacentPosition(angle, x, y) && canPlaceAt( *x, *y );
}

bool Game::addMove(int angle)
{
    int x, y;
    if ( mIntents.empty() ) {
        x = mTankX;
        y = mTankY;
    } else {
        Intent last = mIntents.back();
        x = last.getX();
        y = last.getY();
    }
    if ( canMoveFrom( angle, &x, &y ) ) {
        addMoveInternal( angle, x, y );
        return true;
    }
    return false;
}

void Game::addMoveInternal( int angle, int x, int y )
{
    mIntents.push_back( Intent(MOVE,x,y,angle) );
    setProperty("IntentList", QVariant::fromValue(mIntents));
    emit intentAdded(mIntents.back());
}

void Game::onTankMoved( int x, int y )
{
    mTankX = x;
    mTankY = y;
    if ( mIntents.front().encodedPos() == Intent::encodePos(x,y) ) {
        mIntents.pop_front();
        setProperty("IntentList", QVariant::fromValue(mIntents));
    }
}

void Game::clearIntents()
{
    std::list<Intent>::iterator it;

    for( it=mIntents.begin(); it != mIntents.end(); ++it ) {
        emit intentRemoved( *it );
    }
    mIntents.clear();
    setProperty("IntentList", QVariant::fromValue(mIntents));
}

int Game::getTankX()
{
    return mTankX;
}

int Game::getTankY()
{
    return mTankY;
}

bool Game::getAdjacentPosition( int angle, int *x, int *y )
{
    if ( mBoard ) {
        switch( angle ) {
        case   0: *y -= 1; return true;
        case  90: *x += 1; return true;
        case 180: *y += 1; return true;
        case 270: *x -= 1; return true;
        default:
            ;
        }
    }
    return false;
}

bool Game::canPlaceAt( int x, int y )
{
    switch( mBoard->tileAt(x,y) ) {
    case DIRT:
        return true;
    default:
        ;
    }
    return false;
}

#include <iostream>
#include <QVariant>
#include <QRegion>
#include <QMessageBox>

#include "controller/Game.h"

Game::Game( Board* board )
{
    mBoard = board;
    if ( board ) {
        QObject::connect( board, &Board::tileChanged, this, &Game::onBoardTileChanged);
        mTankX = board->mInitialTankX;
        mTankY = board->mInitialTankY;
        emit tankInitialized( mTankX, mTankY );
    }
    mHandle.game = this;

    setProperty("MoveList", QVariant::fromValue(mMoves));

    mHorizontalPieceAnimation = new QPropertyAnimation( this, "pieceX" );
    mVerticalPieceAnimation   = new QPropertyAnimation( this, "pieceY" );
    QObject::connect(mHorizontalPieceAnimation, &QVariantAnimation::finished,this,&Game::onPieceStopped);
    QObject::connect(mVerticalPieceAnimation,   &QVariantAnimation::finished,this,&Game::onPieceStopped);
    mHorizontalPieceAnimation->setDuration( 1000 );
    mVerticalPieceAnimation->setDuration( 1000 );

    mMovingPieceType = NONE;
    mPieceBoundingRect.setRect(0,0,24,24);
}

GameHandle Game::getHandle()
{
    return mHandle;
}

Board* Game::getBoard()
{
    return mBoard;
}

QVariant Game::getPieceX()
{
    return QVariant( mPieceBoundingRect.left() );
}

QVariant Game::getPieceY()
{
    return QVariant( mPieceBoundingRect.top() );
}

void Game::setPieceX( const QVariant& x )
{
    int xv = x.toInt();
    if ( xv != mPieceBoundingRect.left() ) {
        QRect dirty( mPieceBoundingRect );
        mPieceBoundingRect.moveLeft( xv );
        dirty |= mPieceBoundingRect;
        emit pieceMoved( dirty );
    }
}

void Game::setPieceY( const QVariant& y )
{
    int yv = y.toInt();
    if ( yv != mPieceBoundingRect.top() ) {
        QRect dirty( mPieceBoundingRect );
        mPieceBoundingRect.moveTop( yv );
        dirty |= mPieceBoundingRect;
        emit pieceMoved( dirty );
    }
}

PieceType Game::getMovingPieceType()
{
    return mMovingPieceType;
}

void Game::onPieceStopped()
{
    if ( mMovingPieceType != NONE ) {
        if ( mBoard ) {
            int x = getPieceX().toInt()/24;
            int y = getPieceY().toInt()/24;
            if ( mMovingPieceType == TILE && mBoard->tileAt(x,y) == WATER ) {
                mBoard->setTileAt( TILE_SUNK, x, y );
            } else {
                mBoard->addPiece( mMovingPieceType, x, y );
            }
        }
        mMovingPieceType = NONE;
        emit pieceStopped();
    }
}

bool Game::canMoveFrom( PieceType what, int angle, int *x, int *y, bool canPush ) {
    return getAdjacentPosition(angle, x, y) && canPlaceAt( what, *x, *y );
}

bool Game::canShootFrom( int angle, int *x, int *y ) {
    return getAdjacentPosition(angle, x, y) && canShootThru( angle, *x, *y );
}

bool Game::addMove(int angle)
{
    int x, y;
    if ( mMoves.empty() ) {
        x = mTankX;
        y = mTankY;
    } else {
        Piece last = mMoves.back();
        x = last.getX();
        y = last.getY();
    }
    if ( canMoveFrom( TANK, angle, &x, &y ) ) {
        addMoveInternal( angle, x, y );
        return true;
    }
    return false;
}

void Game::addMoveInternal( int angle, int x, int y )
{
    mMoves.push_back( Piece(MOVE,x,y,angle) );
    setProperty("MoveList", QVariant::fromValue(mMoves));
    emit pieceAdded(mMoves.back());
}

void Game::onTankMoved( int x, int y )
{
    mTankX = x;
    mTankY = y;
    if ( mMoves.front().encodedPos() == Piece::encodePos(x,y) ) {
        mMoves.pop_front();
        setProperty("MoveList", QVariant::fromValue(mMoves));

        if ( mBoard && mBoard->tileAt(x,y) == FLAG ) {
            QMessageBox msgBox;
            msgBox.setText("Level completed!");
            msgBox.exec();
            mBoard->load(":/maps/level2.txt");
            mTankX = mBoard->mInitialTankX;
            mTankY = mBoard->mInitialTankY;
            emit tankInitialized( mTankX, mTankY );
        }
    }
}

void Game::clearMoves()
{
    for( auto it = mMoves.begin(); it != mMoves.end(); ++it ) {
        emit pieceRemoved( *it );
    }
    mMoves.clear();
    setProperty("MoveList", QVariant::fromValue(mMoves));
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
                cout << "move tile to " << toX << "," << toY << std::endl;
                mBoard->erasePieceAt( x, y );
                QPoint p( x*24, y*24 );
                mPieceBoundingRect.moveTopLeft( p );
                mMovingPieceType = type;
                if ( toX != x ) {
                    mHorizontalPieceAnimation->setStartValue( p.x() );
                    mHorizontalPieceAnimation->setEndValue( toX*24 );
                    mHorizontalPieceAnimation->start();
                } else if ( toY != y ) {
                    mVerticalPieceAnimation->setStartValue( p.y() );
                    mVerticalPieceAnimation->setEndValue( toY*24 );
                    mVerticalPieceAnimation->start();
                }
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

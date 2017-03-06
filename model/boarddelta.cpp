#include <iostream>
#include <QVector>

#include "boarddelta.h"

BoardDelta::BoardDelta(QObject *parent) : QObject(parent), mEnabled(false)
{
}

void BoardDelta::init( Board* masterBoard, Board* futureBoard )
{
    mMasterBoard = masterBoard;
    mFutureBoard = futureBoard;
}

const PieceSetManager* BoardDelta::getPieceManager() const
{
    return &mPieceManager;
}

Board* BoardDelta::getFutureBoard()
{
    return mFutureBoard;
}

bool BoardDelta::enabled() const
{
    return mEnabled;
}

void BoardDelta::enable( bool newValue )
{
    if ( newValue != mEnabled ) {
        std::cout << "delta enable " << newValue << std::endl;

        if ( !newValue ) {
            QObject::disconnect( mMasterBoard->getPieceManager(), 0, this, 0 );
            QObject::disconnect( mFutureBoard->getPieceManager(), 0, this, 0 );
            QObject::disconnect( mMasterBoard, &Board::tileChangedAt, this, 0 );
            QObject::disconnect( mFutureBoard, &Board::tileChangedAt, this, 0 );

            mPieceManager.reset(); // reset now for rendering
        } else {
            mFutureBoard->load( mMasterBoard );
            const PieceSetManager* masterManager = mMasterBoard->getPieceManager();
            PieceSetManager*       futureManager = mFutureBoard->getPieceManager();
            QObject::connect( masterManager, &PieceSetManager::insertedAt, this, &BoardDelta::onChangeAt );
            QObject::connect( masterManager, &PieceSetManager::erasedAt,   this, &BoardDelta::onChangeAt );
            QObject::connect( futureManager, &PieceSetManager::insertedAt, this, &BoardDelta::onChangeAt );
            QObject::connect( futureManager, &PieceSetManager::erasedAt,   this, &BoardDelta::onChangeAt );
            QObject::connect( mMasterBoard, &Board::tileChangedAt, this, &BoardDelta::onChangeAt );
            QObject::connect( mFutureBoard, &Board::tileChangedAt, this, &BoardDelta::onChangeAt );
        }
        mEnabled = newValue;
    }
}

void BoardDelta::onChangeAt( int x, int y )
{
    const Piece* masterPiece = mMasterBoard->getPieceManager()->pieceAt( x, y );
    const Piece* futurePiece = mFutureBoard->getPieceManager()->pieceAt( x, y );

    if ( !masterPiece ) {
        if ( !futurePiece ) {
            if ( mMasterBoard->tileAt(x,y) != TILE_SUNK ) {
                if ( mFutureBoard->tileAt(x,y) != TILE_SUNK ) {
                    mPieceManager.eraseAt( x, y );
                } else {
                    mPieceManager.insert( TILE_FUTURE_INSERT, x, y );
                }
            } else if ( mFutureBoard->tileAt(x,y) == TILE_SUNK ) {
                mPieceManager.eraseAt( x, y );
            }
        } else {
            Piece* curDeltaPiece = mPieceManager.pieceAt( x, y );
            if ( curDeltaPiece ) {
                if ( curDeltaPiece->getType() == TILE_FUTURE_INSERT ) {
                    return;
                }
                mPieceManager.erase( curDeltaPiece );
                std::cout << "delta erasedAt " << x << "," << y << " (->INSERT)" << std::endl;
            }
            mPieceManager.insert( TILE_FUTURE_INSERT, x, y, futurePiece->getAngle() );
            std::cout << "delta future INSERT " << x << "," << y << std::endl;
        }
    } else {
        if ( !futurePiece ) {
            mPieceManager.insert( TILE_FUTURE_ERASE, x, y, masterPiece->getAngle() );
            std::cout << "delta future ERASE " << x << "," << y << std::endl;
        } else {
            mPieceManager.eraseAt( x, y );
            std::cout << "delta erasedAt " << x << "," << y << " (same)" << std::endl;
        }
    }
}

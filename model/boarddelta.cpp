#include <iostream>
#include <QVector>

#include "boarddelta.h"
#include "board.h"

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

void BoardDelta::onChangeAt( int col, int row )
{
    const Piece* masterPiece = mMasterBoard->getPieceManager()->pieceAt( col, row );
    const Piece* futurePiece = mFutureBoard->getPieceManager()->pieceAt( col, row );

    if ( !masterPiece ) {
        if ( !futurePiece ) {
            if ( mMasterBoard->tileAt(col,row) != TILE_SUNK ) {
                if ( mFutureBoard->tileAt(col,row) != TILE_SUNK ) {
                    mPieceManager.eraseAt( col, row );
                } else {
                    mPieceManager.insert( TILE_FUTURE_INSERT, col, row );
                }
            } else if ( mFutureBoard->tileAt(col,row) == TILE_SUNK ) {
                mPieceManager.eraseAt( col, row );
            }
        } else {
            Piece* curDeltaPiece = mPieceManager.pieceAt( col, row );
            if ( curDeltaPiece ) {
                if ( curDeltaPiece->getType() == TILE_FUTURE_INSERT ) {
                    return;
                }
                mPieceManager.erase( curDeltaPiece );
            }
            mPieceManager.insert( TILE_FUTURE_INSERT, col, row, futurePiece->getAngle() );
        }
    } else {
        if ( !futurePiece ) {
            mPieceManager.insert( TILE_FUTURE_ERASE, col, row, masterPiece->getAngle() );
        } else {
            mPieceManager.eraseAt( col, row );
        }
    }
}

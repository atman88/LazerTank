#include <iostream>
#include <QVector>

#include "piecedelta.h"

PieceDelta::PieceDelta(QObject *parent) : QObject(parent), mEnabled(false)
{
}

void PieceDelta::init( Board* masterBoard, Board* futureBoard )
{
    mMasterBoard = masterBoard;
    mFutureBoard = futureBoard;
}

const PieceSetManager* PieceDelta::getPieceManager() const
{
    return &mPieceManager;
}

Board* PieceDelta::getFutureBoard()
{
    return mFutureBoard;
}

bool PieceDelta::enabled() const
{
    return mEnabled;
}

void PieceDelta::enable( bool newValue )
{
    if ( newValue != mEnabled ) {
        std::cout << "delta enable " << newValue << std::endl;

        if ( !newValue ) {
            QObject::disconnect( mMasterBoard->getPieceManager(), 0, this, 0 );
            QObject::disconnect( mFutureBoard->getPieceManager(), 0, this, 0 );

            mPieceManager.reset(); // reset now for rendering
        } else {
            mFutureBoard->load( mMasterBoard );
            const PieceSetManager* masterManager = mMasterBoard->getPieceManager();
            PieceSetManager*       futureManager = mFutureBoard->getPieceManager();
            QObject::connect( masterManager, &PieceSetManager::insertedAt, this, &PieceDelta::onChangeAt );
            QObject::connect( masterManager, &PieceSetManager::erasedAt,   this, &PieceDelta::onChangeAt );
            QObject::connect( futureManager, &PieceSetManager::insertedAt, this, &PieceDelta::onChangeAt );
            QObject::connect( futureManager, &PieceSetManager::erasedAt,   this, &PieceDelta::onChangeAt );
        }
        mEnabled = newValue;
    }
}

void PieceDelta::onChangeAt( int x, int y )
{
    const Piece* masterPiece  = mMasterBoard->getPieceManager()->pieceAt( x, y );
    const Piece* changesPiece = mFutureBoard->getPieceManager()->pieceAt( x, y );

    if ( !masterPiece ) {
        if ( !changesPiece ) {
            if ( mPieceManager.eraseAt( x, y ) ) {
                std::cout << "delta erasedAt " << x << "," << y << " (gone)" << std::endl;
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
            mPieceManager.insert( TILE_FUTURE_INSERT, x, y, changesPiece->getAngle() );
            std::cout << "delta future INSERT " << x << "," << y << std::endl;
        }
    } else {
        if ( !changesPiece ) {
            mPieceManager.insert( TILE_FUTURE_ERASE, x, y, masterPiece->getAngle() );
            std::cout << "delta future ERASE " << x << "," << y << std::endl;
        } else {
            mPieceManager.eraseAt( x, y );
            std::cout << "delta erasedAt " << x << "," << y << " (same)" << std::endl;
        }
    }
}

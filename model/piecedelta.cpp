#include <iostream>
#include <QVector>

#include "piecedelta.h"

PieceDelta::PieceDelta(QObject *parent) : QObject(parent)
{
}

void PieceDelta::init( const PieceSetManager* masterManager, PieceSetManager* changesManager )
{
    mMasterManager  = masterManager;
    mChangesManager = changesManager;
}

const PieceSetManager* PieceDelta::getPieceManager() const
{
    return &mPieceManager;
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
            QObject::disconnect( mMasterManager,  0, this, 0 );
            QObject::disconnect( mChangesManager, 0, this, 0 );

            mPieceManager.reset(); // reset now for rendering
        } else {
            mChangesManager->reset( mMasterManager );
            QObject::connect( mMasterManager,  &PieceSetManager::insertedAt, this, &PieceDelta::onChangeAt );
            QObject::connect( mMasterManager,  &PieceSetManager::erasedAt,   this, &PieceDelta::onChangeAt );
            QObject::connect( mChangesManager, &PieceSetManager::insertedAt, this, &PieceDelta::onChangeAt );
            QObject::connect( mChangesManager, &PieceSetManager::erasedAt,   this, &PieceDelta::onChangeAt );
        }
        mEnabled = newValue;
    }
}

void PieceDelta::onChangeAt( int x, int y )
{
    Piece* masterPiece  = mMasterManager->pieceAt(  x, y );
    Piece* changesPiece = mChangesManager->pieceAt( x, y );

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

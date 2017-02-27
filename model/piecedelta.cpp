#include <QVector>

#include "piecedelta.h"

PieceDelta::PieceDelta(QObject *parent) : QObject(parent)
{
}

void PieceDelta::init( PieceSetManager *masterManager, PieceSetManager *changesManager )
{
    mMasterManager  = masterManager;
    mChangesManager = changesManager;
}

bool PieceDelta::update()
{
    if ( mLastMasterTransactionNo  != mMasterManager->getLastTransactionNo()
      || mLastChangesTransactionNo != mChangesManager->getLastTransactionNo() ) {
        mPieceManager.reset();

        const PieceSet* masterSet  = mMasterManager->getPieces();
        const PieceSet* changesSet = mChangesManager->getPieces();
        auto masterIterator  = masterSet->cbegin();
        auto changesIterator = changesSet->cbegin();

        while( masterIterator != masterSet->cend() && changesIterator != changesSet->cend() ) {
            Piece* masterPiece = *masterIterator;
            Piece* changesPiece  = *changesIterator;
            if ( *masterPiece < *changesPiece ) {
                mPieceManager.insert( TILE_FUTURE_ERASE, masterPiece->getX(), masterPiece->getY(), masterPiece->getAngle() );
                ++masterIterator;
                setDirtyFor( masterPiece );
            } else if ( *changesPiece < *masterPiece ) {
                mPieceManager.insert( TILE_FUTURE_INSERT, changesPiece->getX(), changesPiece->getY(), changesPiece->getAngle() );
                ++changesIterator;
                setDirtyFor( changesPiece );
            } else {
                ++masterIterator;
                ++changesIterator;
            }
        }

        while( masterIterator != masterSet->cend() ) {
            Piece* piece = *masterIterator;
            mPieceManager.insert( TILE_FUTURE_ERASE, piece->getX(), piece->getY(), piece->getAngle() );
            ++masterIterator;
            setDirtyFor( piece );
        }

        while( changesIterator != changesSet->cend() ) {
            Piece* piece = *changesIterator;
            mPieceManager.insert( TILE_FUTURE_INSERT, piece->getX(), piece->getY(), piece->getAngle() );
            ++changesIterator;
            setDirtyFor( piece );
        }

        mLastMasterTransactionNo  = mMasterManager->getLastTransactionNo();
        mLastChangesTransactionNo = mChangesManager->getLastTransactionNo();
    }
    return mPieceManager.count() != 0;
}

const PieceSet* PieceDelta::getPieces()
{
    return mPieceManager.getPieces();
}

void PieceDelta::setDirtyFor(Piece *piece)
{
    emit squareDirty( piece->getX(), piece->getY() );
}

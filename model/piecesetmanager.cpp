#include "piecesetmanager.h"

PieceSetManager::PieceSetManager( QObject* parent ) : QObject(parent), mLastTransactionNo(0)
{
}

const PieceSet* PieceSetManager::getPieces() const
{
    return &mPieces;
}

void PieceSetManager::insert( PieceType type, int x, int y, int angle )
{
    mPieces.insert( new SimplePiece( type, x, y, angle ) );
    ++mLastTransactionNo;
    emit insertedAt( x, y );
}

void PieceSetManager::insert( Piece* piece )
{
    insert( piece->getType(), piece->getX(), piece->getY(), piece->getAngle() );
}

PieceType PieceSetManager::typeAt( int x, int y )
{
    SimplePiece pos( NONE, x, y );
    PieceSet::iterator it = mPieces.find( &pos );
    if ( it != mPieces.end() ) {
        return (*it)->getType();
    }
    return NONE;
}

Piece* PieceSetManager::pieceAt( int x, int y )
{
    SimplePiece pos( NONE, x, y );
    PieceSet::iterator it = mPieces.find( &pos );
    if ( it != mPieces.end() ) {
        return *it;
    }
    return 0;
}

void PieceSetManager::erase( Piece *key )
{
    PieceSet::iterator it = mPieces.find( key );
    if ( it != mPieces.end() ) {
        Piece* piece = *it;
        emit erasingAt( piece->getX(), piece->getY() );
        mPieces.erase( it );
        ++mLastTransactionNo;
        delete piece;
    }
}

void PieceSetManager::eraseAt( int x, int y )
{
    SimplePiece pos( NONE, x, y );
    erase( &pos );
}

void PieceSetManager::reset(PieceSetManager *source)
{
    while( !mPieces.empty() ) {
        PieceSet::iterator it = mPieces.end();
        Piece* piece = *--it;
        emit erasingAt( piece->getX(), piece->getY() );
        mPieces.erase( it );
        delete piece;
    }

    if ( source != 0 ) {
        const PieceSet* sourceSet = source->getPieces();
        for( auto it : *sourceSet ) {
            insert( it->getType(), it->getX(), it->getY(), it->getAngle() );
        }
    }
    ++mLastTransactionNo;
}

int PieceSetManager::getLastTransactionNo()
{
    return mLastTransactionNo;
}

int PieceSetManager::count() const
{
    return mPieces.size();
}

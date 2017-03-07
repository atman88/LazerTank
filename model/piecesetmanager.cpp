#include "piecesetmanager.h"

PieceSetManager::PieceSetManager( QObject* parent ) : QObject(parent)
{
}

PieceSetManager::~PieceSetManager()
{
    for( auto it : mPieces ) {
        delete it;
    }
}

const PieceSet* PieceSetManager::getPieces() const
{
    return &mPieces;
}

void PieceSetManager::insert( PieceType type, int col, int row, int angle )
{
    mPieces.insert( new SimplePiece( type, col, row, angle ) );
    emit insertedAt( col, row );
}

void PieceSetManager::insert( Piece* piece )
{
    insert( piece->getType(), piece->getCol(), piece->getRow(), piece->getAngle() );
}

PieceType PieceSetManager::typeAt( int col, int row )
{
    SimplePiece pos( NONE, col, row );
    PieceSet::iterator it = mPieces.find( &pos );
    if ( it != mPieces.end() ) {
        return (*it)->getType();
    }
    return NONE;
}

Piece* PieceSetManager::pieceAt( int col, int row ) const
{
    SimplePiece pos( NONE, col, row );
    PieceSet::iterator it = mPieces.find( &pos );
    if ( it != mPieces.end() ) {
        return *it;
    }
    return 0;
}

bool PieceSetManager::erase( Piece* key )
{
    PieceSet::iterator it = mPieces.find( key );
    if ( it != mPieces.end() ) {
        Piece* piece = *it;
        int col = piece->getCol();
        int row = piece->getRow();

        mPieces.erase( it );
        delete piece;

        emit erasedAt( col, row );
        return true;
    }
    return false;
}

bool PieceSetManager::eraseAt( int col, int row )
{
    SimplePiece pos( NONE, col, row );
    return erase( &pos );
}

void PieceSetManager::reset(const PieceSetManager* source)
{
    while( !mPieces.empty() ) {
        PieceSet::iterator it = mPieces.end();
        Piece* piece = *--it;
        emit erasedAt( piece->getCol(), piece->getRow() );
        mPieces.erase( it );
        delete piece;
    }

    if ( source != 0 ) {
        const PieceSet* sourceSet = source->getPieces();
        for( auto it : *sourceSet ) {
            insert( it->getType(), it->getCol(), it->getRow(), it->getAngle() );
        }
    }
}

int PieceSetManager::size() const
{
    return mPieces.size();
}

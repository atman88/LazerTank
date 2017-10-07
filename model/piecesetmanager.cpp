#include "piecesetmanager.h"

PieceSetManager::PieceSetManager( QObject* parent ) : PieceManager(parent)
{
}

PieceSetManager::~PieceSetManager()
{
    for( auto it : mPieces ) {
        delete it;
    }
}

const PieceSet& PieceSetManager::getPieces() const
{
    return mPieces;
}

void PieceSetManager::insert(PieceType type, ModelPoint point, int angle )
{
    mPieces.insert( new SimplePiece( type, point, angle ) );
    emit insertedAt( point );
}

void PieceSetManager::insert( Piece* piece )
{
    insert( piece->getType(), *piece, piece->getAngle() );
}

PieceType PieceSetManager::typeAt( ModelPoint point )
{
    SimplePiece pos( NONE, point );
    PieceSet::iterator it = mPieces.find( &pos );
    if ( it != mPieces.end() ) {
        return (*it)->getType();
    }
    return NONE;
}

Piece* PieceSetManager::pieceAt( ModelPoint point ) const
{
    SimplePiece pos( NONE, point );
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
        ModelPoint point = *piece;

        mPieces.erase( it );
        delete piece;

        emit erasedAt( point );
        return true;
    }
    return false;
}

bool PieceSetManager::eraseAt( ModelPoint point )
{
    SimplePiece pos( NONE, point );
    return erase( &pos );
}

void PieceSetManager::setAt(PieceType type, ModelPoint point, int angle )
{
    bool changed = false;

    if ( Piece* piece = pieceAt( point ) ) {
        if ( piece->getType() != type ) {
            piece->setType( type );
            changed = true;
        }
        if ( piece->getAngle() != angle ) {
            piece->setAngle( angle );
            changed = true;
        }
    } else {
        insert( type, point, angle );
        changed = true;
    }

    if ( changed ) {
        emit changedAt( point );
    }
}

void PieceSetManager::reset(const PieceSetManager* source)
{
    while( !mPieces.empty() ) {
        PieceSet::iterator it = mPieces.end();
        Piece* piece = *--it;
        ModelPoint point = *piece;
        mPieces.erase( it );
        delete piece;
        emit erasedAt( point );
    }

    if ( source != 0 ) {
        const PieceSet& sourceSet = source->getPieces();
        for( auto it : sourceSet ) {
            insert( it );
        }
    }
}

int PieceSetManager::size() const
{
    return mPieces.size();
}

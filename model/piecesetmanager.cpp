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

void PieceSetManager::insert( PieceType type, const ModelPoint& point, int angle, int pushedId )
{
    mPieces.insert( pushedId ? (new PushedPiece( type, point, angle, pushedId)) : (new SimplePiece( type, point, angle )) );
    emit insertedAt( point );
}

void PieceSetManager::insert( Piece* piece )
{
    insert( piece->getType(), *piece, piece->getAngle(), piece->getPushedId() );
}

PieceType PieceSetManager::typeAt( const ModelPoint& point )
{
    SimplePiece pos( NONE, point );
    auto it = mPieces.find( &pos );
    if ( it != mPieces.end() ) {
        return (*it)->getType();
    }
    return NONE;
}

Piece* PieceSetManager::pieceAt( const ModelPoint& point ) const
{
    SimplePiece pos( NONE, point );
    auto it = mPieces.find( &pos );
    if ( it != mPieces.end() ) {
        return *it;
    }
    return nullptr;
}

bool PieceSetManager::erase( Piece* key )
{
    auto it = mPieces.find( key );
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

bool PieceSetManager::eraseAt( const ModelPoint& point )
{
    SimplePiece pos( NONE, point );
    return erase( &pos );
}

void PieceSetManager::setAt( PieceType type, const ModelPoint& point, int angle, int pushedId )
{
    if ( Piece* piece = pieceAt( point ) ) {
        bool changed = false;

        if ( piece->getType() != type ) {
            piece->setType( type );
            changed = true;
        }
        if ( piece->getAngle() != angle ) {
            piece->setAngle( angle );
            changed = true;
        }

        if ( changed ) {
            emit changedAt( point );
        }
    } else {
        insert( type, point, angle, pushedId );
    }
}

void PieceSetManager::reset( const PieceSetManager* source )
{
    while( !mPieces.empty() ) {
        auto it = mPieces.end();
        Piece* piece = *--it;
        ModelPoint point = *piece;
        mPieces.erase( it );
        delete piece;
        emit erasedAt( point );
    }

    if ( source != nullptr ) {
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

void PieceSetManager::invalidatePushIdDelineation( int delineation )
{
    for( auto it : mPieces ) {
        if ( it->getPushedId() > delineation ) {
            emit changedAt( *it );
        }
    }
}


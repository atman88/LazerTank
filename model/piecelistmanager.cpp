#include "piecelistmanager.h"

PieceListManager::PieceListManager( QObject *parent ) : PieceManager(parent), mSet{nullptr}, mMultiSet{nullptr}
{
}

PieceListManager::~PieceListManager()
{
    delete mSet;
    delete mMultiSet;
    for( auto it : mPieces ) {
        delete it;
    }
}

const PieceList& PieceListManager::getList() const
{
    return mPieces;
}

const PieceSet* PieceListManager::toSet()
{
    if ( !mSet ) {
        mSet = new PieceSet;
        for( auto iterator : mPieces ) {
            mSet->insert( iterator );
        }
    }
    return mSet;
}

const PieceMultiSet* PieceListManager::toMultiSet()
{
    if ( !mMultiSet ) {
        mMultiSet = new PieceMultiSet;
        for( auto iterator : mPieces ) {
            mMultiSet->insert( iterator );
        }
    }
    return mMultiSet;
}

Piece* PieceListManager::addInternal( Piece* piece , bool pushFront )
{
    if ( pushFront ) {
        mPieces.push_front( piece );
    } else {
        mPieces.push_back( piece );
    }
    if ( mSet ) {
        mSet->insert( piece );
    }
    if ( mMultiSet ) {
        mMultiSet->insert( piece );
    }
    emit insertedAt( *piece );
    return piece;
}

Piece* PieceListManager::append( PieceType type, const ModelVector& vector )
{
    return addInternal( new SimplePiece( type, vector ) );
}

Piece* PieceListManager::append( PieceType type, const ModelPoint& point, int angle )
{
    return addInternal( new SimplePiece( type, point, angle ) );
}

Piece* PieceListManager::append( const Piece* source )
{
    if ( auto move = dynamic_cast<const MovePiece*>(source) ) {
        return addInternal( new MovePiece(move) );
    }
    return addInternal( source->getPushedId() > 0 ? (new PushedPiece(source)) : (new SimplePiece(source)) );
}

void PieceListManager::appendList( const PieceList& source )
{
    for( auto it : source ) {
        append( it );
    }
}

Piece* PieceListManager::push_front( PieceType type, const ModelVector& vector, int shotCount, const Piece* pushPiece )
{
    return addInternal( new MovePiece( type, vector, shotCount, pushPiece ), true );
}

bool PieceListManager::eraseInternal( PieceList::iterator it )
{
    if ( it == mPieces.end() ) {
        return false;
    }

    Piece* piece = *it;
    ModelPoint point = *piece;

    mPieces.erase( it );

    if ( mSet ) {
        mSet->erase( piece );
    }

    if ( mMultiSet ) {
        auto pair = mMultiSet->equal_range( piece );
        for( auto sit = pair.first; sit != pair.second; ++sit ) {
            if ( *sit == piece ) {
                mMultiSet->erase(sit);
                break;
            }
        }
    }

    delete piece;
    emit erasedAt( point );
    return true;
}

bool PieceListManager::eraseFront()
{
    return eraseInternal( mPieces.begin() );
}

Piece* PieceListManager::getBack() const
{
    if ( mPieces.empty() ) {
        return nullptr;
    }
    return mPieces.back();
}

Piece* PieceListManager::getBack( int index ) const
{
    if ( 0 <= index && static_cast<unsigned>(index) < mPieces.size() ) {
        auto it = mPieces.cend();
        do {
            --it;
        } while( --index >= 0 );
        return *it;
    }
    return nullptr;
}

Piece* PieceListManager::getFront() const
{
    if ( mPieces.empty() ) {
        return nullptr;
    }
    return mPieces.front();
}

bool PieceListManager::eraseBack()
{
    if ( !mPieces.empty() ) {
        auto it = mPieces.end();
        return eraseInternal( --it );
    }
    return false;
}

void PieceListManager::replaceInternal( Piece* piece, PieceType type, int newAngle )
{
    piece->setType( type );
    if ( newAngle >= 0 ) {
        piece->setAngle( newAngle );
    }
    emit changedAt( *piece );
}

bool PieceListManager::replaceFront( PieceType type, int newAngle )
{
    if ( !mPieces.empty() ) {
        replaceInternal( mPieces.front(), type, newAngle );
        return true;
    }
    return false;
}

bool PieceListManager::replaceBack( PieceType type, int newAngle )
{
    if ( !mPieces.empty() ) {
        replaceInternal( mPieces.back(), type, newAngle );
        return true;
    }
    return false;
}

void PieceListManager::reset( PieceListManager* source, bool copy )
{
    while( eraseBack() ) {
        // continue
    }
    // clear any sets for safety (shouldn't be necessary):
    if ( mSet ) {
        mSet->clear();
    }
    if ( mMultiSet ) {
        mMultiSet->clear();
    }

    if ( source ) {
        appendList( source, copy );
    }
}

void PieceListManager::appendList( PieceListManager* source, bool copy )
{
    PieceList& list = source->mPieces;
    for( auto it = list.begin(); it != list.end(); ) {
        if ( !copy ) {
            addInternal( *it );
            it = list.erase( it );
        } else {
            append( *it );
            ++it;
        }
    }
}

int PieceListManager::size() const
{
    return mPieces.size();
}

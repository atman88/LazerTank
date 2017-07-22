#include "piecelistmanager.h"

PieceListManager::PieceListManager(QObject *parent) : QObject(parent), mSet(0), mMultiSet(0)
{
}

PieceListManager::~PieceListManager()
{
    if ( mSet ) {
        delete mSet;
    }

    if ( mMultiSet ) {
        delete mMultiSet;
    }

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
        for( auto iterator = mPieces.begin(); iterator != mPieces.end(); ++iterator  ) {
            mSet->insert( *iterator );
        }
    }
    return mSet;
}

const PieceMultiSet* PieceListManager::toMultiSet()
{
    if ( !mMultiSet ) {
        mMultiSet = new PieceMultiSet;
        for( auto iterator = mPieces.begin(); iterator != mPieces.end(); ++iterator  ) {
            mMultiSet->insert( *iterator );
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
    emit added( *piece );
    return piece;
}

Piece* PieceListManager::append( PieceType type, ModelPoint point, int angle )
{
    return addInternal( new SimplePiece( type, point, angle ) );
}

Piece* PieceListManager::append( PieceType type, ModelPoint point, int angle, int shotCount, const Piece* pushPiece )
{
    return addInternal( new MovePiece( type, point, angle, shotCount, pushPiece ) );
}

Piece* PieceListManager::append( PieceType type, ModelVector vector, int shotCount, const Piece* pushPiece )
{
    return addInternal( new MovePiece( type, vector, shotCount, pushPiece ) );
}

Piece* PieceListManager::append( const Piece* source )
{
    if ( const MovePiece* move = dynamic_cast<const MovePiece*>(source) ) {
        return addInternal( new MovePiece(move) );
    }
    return addInternal( new SimplePiece(source) );
}

void PieceListManager::append( const PieceList& source )
{
    for( auto it : source ) {
        append( it );
    }
}

Piece* PieceListManager::push_front( PieceType type, ModelVector vector, int shotCount, const Piece* pushPiece )
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
        for( PieceMultiSet::iterator sit = pair.first; sit != pair.second; ++sit ) {
            if ( *sit == piece ) {
                mMultiSet->erase(sit);
                break;
            }
        }
    }

    delete piece;
    emit erased( point );
    return true;
}

bool PieceListManager::eraseFront()
{
    return eraseInternal( mPieces.begin() );
}

Piece* PieceListManager::getBack() const
{
    if ( mPieces.empty() ) {
        return 0;
    }
    return mPieces.back();
}

Piece* PieceListManager::getBack( int index ) const
{
    if ( 0 <= index && ((unsigned) index) < mPieces.size() ) {
        auto it = mPieces.cend();
        do {
            --it;
        } while( --index >= 0 );
        return *it;
    }
    return 0;
}

Piece *PieceListManager::getFront() const
{
    if ( mPieces.empty() ) {
        return 0;
    }
    return mPieces.front();
}

bool PieceListManager::eraseBack()
{
    if ( !mPieces.empty() ) {
        PieceList::iterator it = mPieces.end();
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
    emit changed( *piece );
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

MovePiece* PieceListManager::setShotCountBack( int count )
{
    if ( !mPieces.empty() ) {
        if ( Piece* piece = mPieces.back() ) {
            MovePiece* move = dynamic_cast<MovePiece*>(piece);
            if ( !move ) {
                move = new MovePiece( piece );
                mPieces.remove( piece );
                mPieces.push_back( move );

                // purge any cached sets given they are now stale:
                if ( mSet ) {
                    delete mSet;
                    mSet = 0;
                }
                if ( mMultiSet ) {
                    delete mMultiSet;
                    mMultiSet = 0;
                }
            }
            if ( move->setShotCount( count ) ) {
                emit changed( *piece );
            }
            return move;
        }
    }
    return 0;
}

void PieceListManager::reset( PieceListManager* source )
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
        const PieceList& list = source->getList();
        for( auto it : list ) {
            append( it );
        }
    }
}

int PieceListManager::size() const
{
    return mPieces.size();
}

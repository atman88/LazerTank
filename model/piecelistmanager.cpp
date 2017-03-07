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

const PieceList* PieceListManager::getList() const
{
    return &mPieces;
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

void PieceListManager::appendInternal( Piece* piece )
{
    mPieces.push_back( piece );
    if ( mSet ) {
        mSet->insert( piece );
    }
    if ( mMultiSet ) {
        mMultiSet->insert( piece );
    }
    emit appended( piece->getCol(), piece->getRow() );
}

void PieceListManager::append( PieceType type, int col, int row, int angle, bool hasPush )
{
    appendInternal( new PusherPiece( type, col, row, angle, hasPush ) );
}

void PieceListManager::append( Piece* source )
{
    if ( source->hasPush() ) {
        appendInternal( new PusherPiece(source) );
    } else {
        appendInternal( new SimplePiece(source) );
    }
}

bool PieceListManager::eraseInternal( PieceList::iterator it )
{
    if ( it == mPieces.end() ) {
        return false;
    }

    Piece* piece = *it;
    mPieces.erase( it );

    int col = piece->getCol();
    int row = piece->getRow();

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
    emit erased( col, row );
    return true;
}

bool PieceListManager::eraseFront()
{
    return eraseInternal( mPieces.begin() );
}

bool PieceListManager::eraseBack()
{
    if ( !mPieces.empty() ) {
        PieceList::iterator it = mPieces.end();
        return eraseInternal( --it );
    }
    return false;
}

bool PieceListManager::replaceBack(PieceType type, int newAngle )
{
    if ( !mPieces.empty() ) {
        Piece* piece = mPieces.back();
        piece->setType( type );
        if ( newAngle >= 0 ) {
            piece->setAngle( newAngle );
        }
        emit replaced( piece->getCol(), piece->getRow() );
        return true;
    }
    return false;
}

void PieceListManager::reset( PieceListManager* source )
{
    while( eraseBack() ) {
        // continue
    }

    if ( source ) {
        const PieceList* list = source->getList();
        for( auto it : *list ) {
            append( it );
        }
    }
}

int PieceListManager::size() const
{
    return mPieces.size();
}

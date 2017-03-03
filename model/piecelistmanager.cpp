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
    emit appended( piece->getX(), piece->getY() );
}

void PieceListManager::append( PieceType type, int x, int y, int angle, bool hasPush )
{
    appendInternal( new PusherPiece( type, x, y, angle, hasPush ) );
}

void PieceListManager::append( PieceType type, int x, int y, int angle, int pusheeOffset )
{
    appendInternal( new PusheePiece( type, x, y, angle, pusheeOffset ) );
}

void PieceListManager::append( PieceType type, int x, int y, int angle, QColor* color )
{
    appendInternal( new ColoredPiece( type, x, y, angle, color ) );
}

void PieceListManager::append( PieceType type, int x, int y, int angle, int pusheeOffset, QColor* color )
{
    appendInternal( new ColoredPiece( type, x, y, angle, pusheeOffset, color ) );
}

void PieceListManager::append( Piece* source )
{
    if ( source->hasPush() ) {
        appendInternal( new PusherPiece(source) );
    } else if ( source->getPusheeOffset() ) {
        appendInternal( new PusheePiece(source) );
    } else if ( source->getColor() ) {
        appendInternal( new ColoredPiece(source) );
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

    int x = piece->getX();
    int y = piece->getY();

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
    emit erased( x, y );
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
        emit replaced( piece->getX(), piece->getY() );
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

int PieceListManager::count() const
{
    return mPieces.size();
}

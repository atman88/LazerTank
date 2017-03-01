#include "piecelistmanager.h"

PieceListManager::PieceListManager(QObject *parent) : QObject(parent), mSet(0)
{
}

const PieceList* PieceListManager::getList() const
{
    return &mPieces;
}

const PieceSet *PieceListManager::toSet()
{
    if ( !mSet ) {
        mSet = new PieceSet;
        for( auto iterator = mPieces.begin(); iterator != mPieces.end(); ++iterator  ) {
            mSet->insert( *iterator );
        }
    }
    return mSet;
}

void PieceListManager::append( PieceType type, int x, int y, int angle, bool hasPush )
{
    PusherPiece* piece = new PusherPiece( type, x, y, angle, hasPush );
    mPieces.push_back( piece );
    if ( mSet ) {
        mSet->insert( piece );
    }
    emit appended( x, y );
}

void PieceListManager::append( Piece* piece )
{
    append( piece->getType(), piece->getX(), piece->getY(), piece->getAngle(), piece->hasPush() );
}

void PieceListManager::append( PieceType type, int x, int y, int angle, int pusheeOffset )
{
    PusheePiece* piece = new PusheePiece( type, x, y, angle, pusheeOffset );
    mPieces.push_back( piece );
    if ( mSet ) {
        mSet->insert( piece );
    }
    emit appended( x, y );
}

bool PieceListManager::eraseFront()
{
    if ( !mPieces.empty() ) {
        Piece* piece = mPieces.front();
        int x = piece->getX();
        int y = piece->getY();

        mPieces.pop_front();
        if ( mSet ) {
            mSet->erase( piece );
        }
        delete piece;
        emit erased( x, y );
        return true;
    }
    return false;
}

bool PieceListManager::eraseBack()
{
    if ( !mPieces.empty() ) {
        Piece* piece = mPieces.back();
        int x = piece->getX();
        int y = piece->getY();

        mPieces.pop_back();
        if ( mSet ) {
            mSet->erase( piece );
        }
        delete piece;
        emit erased( x, y );
        return true;
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

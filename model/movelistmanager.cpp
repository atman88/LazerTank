#include "movelistmanager.h"

MoveListManager::MoveListManager( QObject* parent ) : PieceListManager(parent)
{
}

MovePiece* MoveListManager::append( PieceType type, ModelPoint point, int angle, int shotCount, const Piece* pushPiece )
{
    MovePiece* move = new MovePiece( type, point, angle, shotCount, pushPiece );
    addInternal( move );
    return move;
}

MovePiece* MoveListManager::append( PieceType type, ModelVector vector, int shotCount, const Piece* pushPiece )
{
    MovePiece* move = new MovePiece( type, vector, shotCount, pushPiece );
    addInternal( move );
    return move;
}

MovePiece* MoveListManager::setShotCountBack( int count )
{
    if ( !mPieces.empty() ) {
        PieceList::iterator it = mPieces.end();
        Piece* piece = *--it;
        MovePiece* move = dynamic_cast<MovePiece*>(piece);
        if ( !move ) {
            move = new MovePiece( piece );
            mPieces.erase( it );
            delete piece;
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
            emit changedAt( *move );
        }
        return move;
    }
    return 0;
}

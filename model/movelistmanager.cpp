#include <iostream>
#include "movelistmanager.h"
#include "controller/gameregistry.h"
#include "controller/movecontroller.h"

MoveListManager::MoveListManager( PieceType initialFocus, QObject* parent ) : PieceListManager(parent), mInitialFocus(initialFocus)
{
}

void MoveListManager::setInitialFocus( PieceType initialFocus )
{
    mInitialFocus = initialFocus;
#ifndef QT_NO_DEBUG
    if ( !mPieces.empty() ) {
        std::cout << "** setInitialFocus called on non-empty list" << std::endl;
    }
#endif // QT_NO_DEBUG
}

MovePiece* MoveListManager::append( PieceType type, const ModelPoint& point, int angle, int shotCount, const Piece* pushPiece )
{
    auto move = new MovePiece( type, point, angle, shotCount, pushPiece );
    addInternal( move );
    return move;
}

MovePiece* MoveListManager::append( PieceType type, const ModelVector& vector, int shotCount, const Piece* pushPiece )
{
    auto move = new MovePiece( type, vector, shotCount, pushPiece );
    addInternal( move );
    return move;
}

MovePiece* MoveListManager::setShotCountBack( int count )
{
    if ( !mPieces.empty() ) {
        auto it = mPieces.end();
        Piece* piece = *--it;
        auto move = dynamic_cast<MovePiece*>(piece);
        if ( !move ) {
            move = new MovePiece( piece );
            mPieces.erase( it );
            delete piece;
            mPieces.push_back( move );

            // purge any cached sets given they are now stale:
            if ( mSet ) {
                delete mSet;
                mSet = nullptr;
            }
            if ( mMultiSet ) {
                delete mMultiSet;
                mMultiSet = nullptr;
            }
        }
        if ( move->setShotCount( count ) ) {
            emit changedAt( *move );
        }
        return move;
    }
    return nullptr;
}

ModelVector MoveListManager::getInitialVector()
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        return registry->getMoveController().getBaseFocusVector( mInitialFocus );
    }
    std::cout << "*** getInitialVector: registry unreachable" << std::endl;
    return ModelVector();
}

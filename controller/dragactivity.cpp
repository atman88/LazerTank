#include "dragactivity.h"
#include "controller/gameregistry.h"
#include "controller/game.h"
#include "controller/movecontroller.h"
#include "model/piecelistmanager.h"
#include "model/tank.h"

DragActivity::DragActivity( QObject *parent ) : QObject(parent), mChanged(false)
{
}

void DragActivity::start( ModelPoint startPoint )
{
    mFocusPoint = startPoint;
    mChanged = false;
    setState( Qt::CrossCursor );
}

void DragActivity::setState( Qt::CursorShape shape )
{
    if ( mCursor.shape() != shape ) {
        mCursor.setShape( shape );
        emit stateChanged();
    }
}

const QCursor* DragActivity::getCursor() const
{
    if ( mFocusPoint.isNull() || mCursor.shape() == Qt::ArrowCursor ) {
        return 0;
    }
    return &mCursor;
}

ModelPoint DragActivity::getFocusPoint() const
{
    return mFocusPoint;
}

void DragActivity::onDragTo( ModelPoint p )
{
    if ( mFocusPoint.isNull() ) {
        setState( Qt::ArrowCursor );
    } else if ( p == mFocusPoint ) {
        setState( Qt::CrossCursor );
    } else if ( GameRegistry* registry = getRegistry(this) ) {
        Game& game = registry->getGame();

        for( int angle : { 0, 90, 180, 270 } ) {
            ModelPoint toPoint( mFocusPoint );
            if ( game.canMoveFrom( TANK, angle, &toPoint, true ) ) {
                if ( toPoint == p ) {
                    PieceListManager& moves = registry->getMoveController().getMoves();
                    Piece* lastMove = moves.getBack();
                    Piece* prevMove = moves.getBack(1);
                    if ( lastMove && mFocusPoint.equals( *lastMove ) && prevMove && toPoint.equals( *prevMove ) ) {
                        moves.eraseBack();
                        moves.replaceBack( MOVE_HIGHLIGHT );
                    } else {
                        if ( moves.size() ) {
                            moves.replaceBack( MOVE, angle );
                        } else {
                            // insert an initial rotation if appropriate:
                            Tank& tank = registry->getTank();
                            if ( tank.getRotation() != angle ) {
                                moves.append( MOVE, tank.getPoint(), angle );
                            }
                        }
                        moves.append( MOVE_HIGHLIGHT, toPoint, angle );
                    }
                    mFocusPoint = toPoint;
                    mChanged = true;
                    setState( Qt::CrossCursor );
                    return;
                }
            }
        }
        setState( Qt::ForbiddenCursor );
    }
}

void DragActivity::stop()
{
    // If we've changed it by dragging and effectively cancelled the moves by erasing up to the tank square, then
    // erase any single move given it is only a left-over rotation:
    if ( mChanged ) {
        if ( GameRegistry* registry = getRegistry(this) ) {
            PieceListManager& moves = registry->getMoveController().getMoves();
            if ( moves.size() == 1 && registry->getTank().getPoint().equals( *moves.getBack() ) ) {
                moves.eraseBack();
            }
        }
    }
    mFocusPoint.setNull();
    setState( Qt::ArrowCursor );
}

#include "dragactivity.h"
#include "controller/gameregistry.h"
#include "controller/game.h"
#include "controller/movecontroller.h"
#include "model/piecelistmanager.h"
#include "model/tank.h"

DragActivity::DragActivity( QObject *parent ) : QObject(parent), mState(Inactive), mChanged(false)
{
}

void DragActivity::start( ModelPoint startPoint )
{
    mFocusPoint = startPoint;
    mChanged = false;
    setState( Selecting );
}

void DragActivity::setState(DragState state )
{

    if ( mState != state ) {
        mState = state;
        emit stateChanged();
    }
}

DragState DragActivity::getState() const
{
    return mState;
}

ModelPoint DragActivity::getFocusPoint() const
{
    return mFocusPoint;
}

void DragActivity::onDragTo( ModelPoint p )
{
    if ( mFocusPoint.isNull() ) {
        setState( Inactive );
    } else if ( p == mFocusPoint ) {
        setState( Selecting );
    } else if ( GameRegistry* registry = getRegistry(this) ) {
        Game& game = registry->getGame();

        for( int angle = 0; angle < 360; angle += 90 ) {
            ModelPoint toPoint( mFocusPoint );
            Piece* pushPiece = 0;
            if ( game.canMoveFrom( TANK, angle, &toPoint, true, &pushPiece ) ) {
                if ( toPoint == p ) {
                    MoveController& moveController = registry->getMoveController();
                    PieceListManager& moves = moveController.getMoves();
                    Piece* lastMove = moves.getBack();
                    const ModelPoint* prevMovePoint;
                    if ( Piece* prevMove = moves.getBack(1) ) {
                        prevMovePoint = prevMove;
                    } else {
                        prevMovePoint = &registry->getTank().getPoint();
                    }
                    if ( lastMove && mFocusPoint.equals( *lastMove ) && prevMovePoint->equals( toPoint ) ) {
                        game.undoLastMove();
                        // if only a simple rotation remains then remove it too:
                        if ( moves.size() == 1 ) {
                            lastMove = moves.getBack();
                            if ( registry->getTank().getPoint().equals( *lastMove ) ) {
                                game.undoLastMove();
                            }
                        }
                    } else {
                        moveController.move( angle, false );
                        lastMove = moves.getBack();
                        if ( lastMove && !p.equals( *lastMove) ) {
                            moveController.move( angle, false );
                        }
                    }
                    mFocusPoint = toPoint;
                    mChanged = true;
                    setState( Selecting );
                    return;
                }
            }
        }
        setState( Forbidden );
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
    setState( Inactive );
}

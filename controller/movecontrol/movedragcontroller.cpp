#include "../movecontroller.h"
#include "gameregistry.h"
#include "game.h"
#include "movecontroller.h"
#include "pathfindercontroller.h"
#include "model/piecelistmanager.h"
#include "model/tank.h"

MoveDragController::MoveDragController( QObject *parent ) : MoveBaseController(parent), mDragState(Inactive), mChanged(false)
{
}

void MoveDragController::init( GameRegistry* registry )
{
    MoveBaseController::init( registry );
    QObject::connect( &registry->getPathFinderController(), &PathFinderController::pathFound, this, &MoveDragController::onPathFound );
}

void MoveDragController::onBoardLoaded( Board* board )
{
    MoveBaseController::onBoardLoaded( board );
    setDragState( Inactive );
}

void MoveDragController::dragStart( ModelPoint selectedPoint )
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        mChanged = false;
        ModelPoint startPoint = getFocusVector();

        PathSearchAction& pathToAction = registry->getPathToAction();
        if ( selectedPoint.equals( startPoint ) ) {
            setDragState( Selecting );
        } else {
            Board* board = registry->getGame().getBoard(true);
            if ( /*Piece* piece =*/ board->getPieceManager().pieceAt( selectedPoint ) ) {
                setDragState( Forbidden );
            } else switch( board->tileAt( selectedPoint ) ) {
            case DIRT:
            case TILE_SUNK:
            case FLAG:
                pathToAction.setCriteria( mFocus, selectedPoint );
                registry->getPathFinderController().doAction( &pathToAction );
                setDragState( Searching );
                break;
            default:
                setDragState( Forbidden );
            }
        }
    }
}

void MoveDragController::move( int direction, bool doWakeup )
{
    if ( mDragState != Inactive ) {
        // Prevent moving beyond the current square given the mouse cursor would get out of sync
        // (Need a means of moving the mouse cursor in order to remove this limitation)
        ModelVector focusVector = getFocusVector();
        if ( focusVector.mAngle == direction ) {
            return;
        }
    }

    MoveBaseController::move( direction, doWakeup );
}

bool MoveDragController::canWakeup()
{
    if ( mDragState != Inactive ) {
        return false;
    }
    return MoveBaseController::canWakeup();
}

void MoveDragController::setDragState( DragState state )
{
    if ( mDragState != state ) {
        mDragState = state;
        emit dragStateChanged( state );
    }
}

DragState MoveDragController::getDragState() const
{
    return mDragState;
}

void MoveDragController::onPathFound( PieceListManager* path, PathSearchAction* action )
{
    MoveBaseController::onPathFound( path, action );
    if ( mDragState == Searching ) {
        setDragState( Selecting );
    }
}

void MoveDragController::onDragTo( ModelPoint p )
{
    if ( mDragState == Inactive || mDragState == Searching ) {
        return;
    }

    ModelPoint focusPoint = getFocusVector();
    if ( focusPoint.isNull() ) {
        setDragState( Inactive );
    } else if ( p == focusPoint ) {
        setDragState( Selecting );
    } else if ( GameRegistry* registry = getRegistry(this) ) {
        Game& game = registry->getGame();

        for( int angle = 0; angle < 360; angle += 90 ) {
            ModelPoint toPoint( focusPoint );
            Piece* pushPiece = 0;
            if ( game.canMoveFrom( TANK, angle, &toPoint, true, &pushPiece ) ) {
                if ( toPoint == p ) {
                    Piece* lastMove = mMoves.getBack();
                    const ModelPoint* prevMovePoint;
                    if ( Piece* prevMove = mMoves.getBack(1) ) {
                        prevMovePoint = prevMove;
                    } else {
                        prevMovePoint = &registry->getTank().getPoint();
                    }
                    setDragState( Selecting );
                    if ( lastMove && focusPoint.equals( *lastMove ) && prevMovePoint->equals( toPoint ) ) {
                        undoLastMove();
                        // if only a simple rotation remains then remove it too:
                        if ( mMoves.size() == 1 ) {
                            lastMove = mMoves.getBack();
                            if ( registry->getTank().getPoint().equals( *lastMove ) ) {
                                undoLastMove();
                            }
                        }
                    } else {
                        MoveBaseController::move( angle );
                        lastMove = mMoves.getBack();
                        if ( lastMove && !p.equals( *lastMove ) ) {
                            MoveBaseController::move( angle );
                        }
                    }
                    mChanged = true;
                    return;
                }
            }
        }
        setDragState( Forbidden );
    }
}

void MoveDragController::dragStop()
{
    if ( mDragState != Inactive ) {
        // If we've changed it by dragging and effectively cancelled the moves by erasing up to the tank square, then
        // erase any single move given it is only a left-over rotation:
        if ( mChanged && mMoves.size() == 1 ) {
            if ( GameRegistry* registry = getRegistry(this) ) {
                if ( registry->getTank().getPoint().equals( *mMoves.getBack() ) ) {
                    mMoves.eraseBack();
                }
            }
        }
        setDragState( Inactive );
        wakeup();
    }
}

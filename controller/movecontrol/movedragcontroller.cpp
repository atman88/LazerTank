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

void MoveDragController::onBoardLoaded( Board& board )
{
    MoveBaseController::onBoardLoaded( board );
    setDragState( Inactive );
}

void MoveDragController::dragStart( ModelPoint selectedPoint )
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        mChanged = false;
        mPreviousCoord = QPoint();
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

/**
 * @brief Determine whether the given coordinate is leaning toward a direction relative to the given square
 * @param coord The pixel coordinate to check
 * @param square The model square to check against
 * @return The angle (0,90,180 or 270) the point is leaning toward or -1 if no angle is favored
 */
static int coordLeaning( QPoint coord, ModelPoint square )
{
    int rotation = -1;
    QPoint relative( coord );
    relative -= square.toViewCenterSquare();
    if ( std::abs(relative.x()) < 24/4 ) {
        if ( std::abs(relative.y()) >= 24/4 ) {
            rotation = (relative.y() > 0) ? 180 : 0;
        }
    } else if ( std::abs(relative.y()) < 24/4 ) {
        rotation = (relative.x() > 0) ? 90 : 270;
    }
    return rotation;
}

void MoveDragController::onDragTo( QPoint coord )
{
    if ( mDragState == Inactive || mDragState == Searching ) {
        return;
    }

    ModelPoint p( coord );
    ModelVector focusVector = getFocusVector();
    if ( focusVector.isNull() ) {
        setDragState( Inactive );
        return;
    }

    if ( p == focusVector ) {
        setDragState( Selecting );
    } else if ( GameRegistry* registry = getRegistry(this) ) {
        Game& game = registry->getGame();

        for( int angle = 0; angle < 360; angle += 90 ) {
            ModelPoint toPoint( focusVector );
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
                    if ( lastMove && !lastMove->getShotCount()
                      && focusVector.ModelPoint::equals( *lastMove ) && prevMovePoint->equals( toPoint ) ) {
                        undoLastMove();
                        // if only a simple rotation remains then remove it too:
                        if ( (lastMove = mMoves.getBack()) ) {
                            if ( !lastMove->getShotCount() && registry->getTank().getPoint().equals( *lastMove ) ) {
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

    // check for rotation change:
    int rotation = coordLeaning( coord, focusVector );
    if ( rotation >= 0 && rotation != focusVector.mAngle && rotation != coordLeaning( mPreviousCoord, focusVector ) ) {
        MoveBaseController::move( rotation );
    }

    mPreviousCoord = coord;
}

void MoveDragController::dragStop()
{
    if ( mDragState != Inactive ) {
        // If we've changed it by dragging and effectively cancelled the moves by erasing up to the tank square, then
        // erase any single move given it is only a left-over rotation:
        if ( mChanged ) {
            if ( Piece* piece = mMoves.getBack() ) {
                if ( !piece->getShotCount() ) {
                    if ( GameRegistry* registry = getRegistry(this) ) {
                        if ( registry->getTank().getPoint().equals( *mMoves.getBack() ) ) {
                            mMoves.eraseBack();
                        }
                    }
                }
            }
        }
        setDragState( Inactive );
        wakeup();
    }
}

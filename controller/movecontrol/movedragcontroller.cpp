#include "../movecontroller.h"
#include "gameregistry.h"
#include "game.h"
#include "movecontroller.h"
#include "pathfindercontroller.h"
#include "model/piecelistmanager.h"
#include "model/tank.h"

MoveDragController::MoveDragController( QObject *parent ) : MoveBaseController(parent), mDragState(Inactive),
  mTileDragFocusAngle(-1), mTileDragAngleMask(0), mChanged(false)
{
}

void MoveDragController::init( GameRegistry* registry )
{
    MoveBaseController::init( registry );
    PathFinderController& controller = registry->getPathFinderController();
    QObject::connect( &controller, &PathFinderController::pathFound,  this, &MoveDragController::onPathFound  );
    QObject::connect( &controller, &PathFinderController::testResult, this, &MoveDragController::onTestResult );
    mTileDragTestResult.setParent(this);
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

        if ( selectedPoint.equals( getFocusVector() ) ) {
            setDragState( DraggingTank );
        } else {
            Board* board = registry->getGame().getBoard(true);
            if ( Piece* piece = board->getPieceManager().pieceAt( selectedPoint ) ) {
                if ( mTileDragTestCriteria.setTileDragCriteria( mFocus, piece, &mTileDragTestResult ) ) {
                    setDragState( Searching );
                    registry->getPathFinderController().testCriteria( &mTileDragTestCriteria );
                } else {
                    setDragState( Forbidden );
                }
            } else switch( board->tileAt( selectedPoint ) ) {
            case DIRT:
            case TILE_SUNK:
            case FLAG:
            {   PathSearchAction& pathToAction = registry->getPathToAction();
                pathToAction.setCriteria( mFocus, selectedPoint );
                registry->getPathFinderController().doAction( &pathToAction );
                setDragState( Searching );
            }
                break;
            default:
                setDragState( Forbidden );
            }
        }
    }
}

unsigned MoveDragController::getDragTileAngleMask() const
{
    return mTileDragAngleMask;
}

ModelPoint MoveDragController::getDragTilePoint() const
{
    return mTileDragTestCriteria.getTargetPoint();
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
        if ( state == Inactive ) {
            mTileDragFocusAngle = -1;
            mTileDragAngleMask = 0;
        }
        mDragState = state;
        emit dragStateChanged( state );
    }
}

bool MoveDragController::setTileDragFocusAngle( int angle )
{
    if ( angle >= 0 && !(mTileDragAngleMask & (1 << (angle / 90)) ) ) {
        angle = -1;
    }
    if ( angle != mTileDragFocusAngle ) {
        mTileDragFocusAngle = angle;
        emit tileDragFocusChanged( angle );
        return true;
    }
    return false;
}

DragState MoveDragController::getDragState() const
{
    return mDragState;
}

void MoveDragController::onPathFound( PieceListManager* path, PathSearchCriteria* criteria )
{
    MoveBaseController::onPathFound( path, criteria );
    if ( mDragState == Searching ) {
        if ( criteria->getCriteriaType() == PathSearchCriteria::TileDragTestCriteria ) {
            MoveBaseController::move( mTileDragFocusAngle );
            setDragState( DraggingTile );
        } else {
            setDragState( DraggingTank );
        }
    }
}

void MoveDragController::onTestResult( bool reachable, PathSearchCriteria* criteria )
{
    if ( mDragState == Searching ) {
        if ( reachable ) {
            // verify this is a tile drag test we initiated
            if ( criteria->getTileDragTestResult() == &mTileDragTestResult ) {
                auto approaches = mTileDragTestResult.mPossibleApproaches;
                mTileDragAngleMask = 0;
                for( int angleNo = 4; --angleNo >= 0; ) {
                    ModelPoint point = mTileDragTestCriteria.getTargetPoint();
                    if ( getAdjacentPosition( (angleNo * 90 + 180) % 360, &point ) ) {
                        if ( approaches.find( point ) != approaches.end() ) {
                            mTileDragAngleMask |= 1 << angleNo;
                        }
                    }
                }

                setDragState( DraggingTile );
                return;
            }
        }
        setDragState( Inactive );
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
    int dx = std::abs( relative.x() );
    int dy = std::abs( relative.y() );
    if ( dx < dy ) {
        if ( dy > 2 ) {
            rotation = (relative.y() > 0) ? 180 : 0;
        }
    } else if ( dy < dx ) {
        if ( dx > 2 ) {
            rotation = (relative.x() > 0) ? 90 : 270;
        }
    }
    return rotation;
}

void MoveDragController::onDragTo( QPoint coord )
{
    ModelPoint p( coord );
    ModelVector focusVector = getFocusVector();
    if ( focusVector.isNull() ) { // safety
        setDragState( Inactive );
        return;
    }

    switch( mDragState ) {
    case Inactive:
    case Searching:
        return;

    case Forbidden:
    case DraggingTank:
        if ( p == focusVector ) {
            setDragState( DraggingTank );
        } else if ( GameRegistry* registry = getRegistry(this) ) {
            for( int angle = 0; angle < 360; angle += 90 ) {
                ModelPoint toPoint( focusVector );
                Piece* pushPiece = 0;
                if ( registry->getGame().canMoveFrom( TANK, angle, &toPoint, true, &pushPiece ) ) {
                    if ( toPoint == p ) {
                        Piece* lastMove = mMoves.getBack();
                        const ModelPoint* prevMovePoint;
                        if ( Piece* prevMove = mMoves.getBack(1) ) {
                            prevMovePoint = prevMove;
                        } else {
                            prevMovePoint = &registry->getTank().getPoint();
                        }
                        setDragState( DraggingTank );
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
    {   int rotation = coordLeaning( coord, focusVector );
        if ( rotation >= 0 && rotation != focusVector.mAngle && rotation != coordLeaning( mPreviousCoord, focusVector ) ) {
            MoveBaseController::move( rotation );
        }
    }
        break;

    case DraggingTile:
        if ( p.equals( mTileDragTestCriteria.getTargetPoint() ) ) {
            if ( setTileDragFocusAngle( coordLeaning( coord, p ) ) && mTileDragFocusAngle >= 0 ) {
                if ( GameRegistry* registry = getRegistry(this) ) {
                    if ( registry->getPathFinderController().buildTilePushPath(
                      ModelVector(mTileDragTestCriteria.getTargetPoint(), mTileDragFocusAngle) ) ) {
                        setDragState( Searching );
                    }
                }
            }
        }
        break;
    }

    mPreviousCoord = coord;
}

void MoveDragController::dragStop()
{
    switch( mDragState ) {
    case DraggingTank:
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
        break;

    case DraggingTile:
        setDragState( Inactive );
        wakeup();
        break;

    default:
        setDragState( Inactive );
    }
}

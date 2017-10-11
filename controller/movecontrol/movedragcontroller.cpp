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
    if ( mDragState == Inactive ) {
        MoveBaseController::move( direction, doWakeup );
    } else {
        // Prevent moving beyond the current square given the mouse cursor would get out of sync
        // (Need a means of moving the mouse cursor in order to remove this limitation)
        ModelVector focusVector = getDragFocusVector();
        if ( focusVector.mAngle == direction ) {
            return;
        }

        moveInternal( focusVector, mDragMoves, direction, false );
    }
}

void MoveDragController::undoLastMove()
{
    if ( !mDragMoves.size() ) {
        MoveBaseController::undoLastMove();
    } else {
        undoLastMoveInternal( mDragMoves );
        mDragMoves.replaceBack( MOVE_HIGHLIGHT );
    }
}

void MoveDragController::undoMoves()
{
    while( mDragMoves.size() ) {
        undoLastMoveInternal( mDragMoves );
    }
    MoveBaseController::undoMoves();
}

void MoveDragController::setFocus(PieceType what)
{
    if ( mDragMoves.size() > 1 ) {
        mDragMoves.replaceBack( what == TANK ? MOVE : MOVE_HIGHLIGHT );
    }
    MoveBaseController::setFocus( what );
}

void MoveDragController::fire( int count )
{
    if ( mDragState != Inactive ) {
        fireInternal( getFocusVector(), mDragMoves, count );
    } else {
        MoveBaseController::fire( count );
    }
}

void MoveDragController::setDragState( DragState state )
{
    if ( mDragState != state ) {
        if ( state == Inactive ) {
            mTileDragFocusAngle = -1;
            mTileDragAngleMask = 0;

            if ( mDragMoves.size() ) {
                for( auto it : mDragMoves.getList() ) {
                    if ( int uid = it->getShotPathUID() ) {
                        for( auto fsit : mFutureShots.getPaths() ) {
                            if ( fsit.getUID() >= uid ) {
                                mFutureShots.invalidate( fsit );
                            }
                        }
                        break;
                    }
                }

                if ( mFocus == TANK ) {
//                if ( GameRegistry* registry = getRegistry(this) ) {
//                    if ( !criteria->getStartPoint().equals( registry->getTank().getPoint() ) ) {
//                        std::cout << "* applyPathUsingCriteria: ingoring stale path" << std::endl;
//                        return false;
//                    }
//                    registry->getGame().endMoveDeltaTracking();
//                } else {
//                    std::cout << "*** applyPathUsingCriteria: registry error" << std::endl;
//                    return false;
//                }
                    mMoves.reset( &mDragMoves, false );
                } else {
//                if ( !criteria->getStartPoint().equals( getFocusVector() ) ) {
//                    std::cout << "* applyPathUsingCriteria: stale start point" << std::endl;
//                    return false;
//                }
                    mMoves.replaceBack( MOVE );
                    mMoves.append( &mDragMoves, false );
                }
                mMoves.replaceBack( MOVE_HIGHLIGHT );
            }
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

ModelVector MoveDragController::getDragFocusVector() const
{
    if ( Piece* move = mDragMoves.getBack() ) {
        if ( mFocus == MOVE || move->getType() == MOVE_HIGHLIGHT ) {
            return *move;
        } else {
            return *mDragMoves.getFront();
        }
    }
    return getFocusVector();
}

void MoveDragController::onPathFound( PieceListManager* path, PathSearchCriteria* criteria )
{
    if ( mDragState == Inactive ) {
        applyPathUsingCriteria( path, criteria );
    } else {
        mDragMoves.reset( path );
        mDragMoves.replaceBack( MOVE_HIGHLIGHT );
        if ( criteria->getCriteriaType() == PathSearchCriteria::TileDragTestCriteria ) {
            moveInternal( getDragFocusVector(), mDragMoves, mTileDragFocusAngle, false );
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
    ModelVector focusVector = getDragFocusVector();
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
                        Piece* lastMove = mDragMoves.getBack();
                        int trimCount = 1;
                        const ModelPoint* prevMovePoint;
                        Piece* prevMove = mDragMoves.getBack(1);
                        if ( prevMove && prevMove->ModelPoint::equals( *lastMove ) ) {
                            prevMove = mDragMoves.getBack(++trimCount);
                        }
                        if ( prevMove ) {
                            prevMovePoint = prevMove;
                        } else {
                            prevMovePoint = &registry->getTank().getPoint();
                        }
                        setDragState( DraggingTank );
                        if ( lastMove && !lastMove->getShotCount()
                          && focusVector.ModelPoint::equals( *lastMove ) && prevMovePoint->equals( toPoint ) ) {
                            while( --trimCount >= 0 ) {
                                undoLastMoveInternal( mDragMoves );
                            }
                            if ( (lastMove = mDragMoves.getBack()) ) {
                                // if only a simple rotation remains then remove it too:
                                if ( mDragMoves.size() == 1 ) {
                                    if ( !lastMove->getShotCount() && focusVector.ModelPoint::equals( *lastMove ) ) {
                                        undoLastMoveInternal( mDragMoves );
                                    }
                                } else {
                                    mDragMoves.replaceBack( MOVE_HIGHLIGHT );
                                }
                            }
                        } else {
                            moveInternal( getDragFocusVector(), mDragMoves, angle, false );
                            lastMove = mDragMoves.getBack();
                            if ( lastMove && !p.equals( *lastMove ) ) {
                                moveInternal( getDragFocusVector(), mDragMoves, angle, false );
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
            moveInternal( getDragFocusVector(), mDragMoves, rotation, false );
        }
    }
        break;

    case DraggingTile:
        if ( p.equals( mTileDragTestCriteria.getTargetPoint() ) ) {
            if ( GameRegistry* registry = getRegistry(this) ) {
                if ( setTileDragFocusAngle( coordLeaning( coord, p ) ) ) {
                    if ( mTileDragFocusAngle >= 0 ) {
                        if ( registry->getPathFinderController().buildTilePushPath(
                                 ModelVector(mTileDragTestCriteria.getTargetPoint(), mTileDragFocusAngle) ) ) {
                            setDragState( Searching );
                        }
                    } else {
                        while( mDragMoves.size() ) {
                            undoLastMoveInternal( mDragMoves );
                        }
                    }
                }
            }
        } else {
            ModelPoint toPoint = mTileDragTestCriteria.getTargetPoint();
            if ( getAdjacentPosition( mTileDragFocusAngle, &toPoint ) && p.equals( toPoint ) ) {
                setDragState( DraggingTank );
                onDragTo( coord );
                return;
            }
        }
        break;
    }

    mPreviousCoord = coord;
}

void MoveDragController::dragStop()
{
    if ( mDragState == DraggingTank ) {
        // If we've changed it by dragging and effectively cancelled the moves by erasing up to the tank square, then
        // erase any single move given it is only a left-over rotation:
        if ( mChanged && mDragMoves.size() == 1 ) {
            if ( Piece* piece = mDragMoves.getBack() ) {
                if ( !piece->getShotCount() && getFocusVector().ModelPoint::equals( *piece ) ) {
                    mDragMoves.eraseBack();
                }
            }
        }
    }
    setDragState( Inactive );
    wakeup();
}

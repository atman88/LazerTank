#include <iostream>
#include "../movecontroller.h"
#include "gameregistry.h"
#include "game.h"
#include "movecontroller.h"
#include "pathfindercontroller.h"
#include "model/piecelistmanager.h"
#include "model/tank.h"

MoveDragController::MoveDragController( QObject *parent ) : MoveBaseController(parent), mDragState{Inactive},
  mTileDragFocusAngle{-1}, mTileDragAngleMask{0}, mMinPushedId{-1}, mChanged{false}
{
}

void MoveDragController::init( GameRegistry* registry )
{
    MoveBaseController::init( registry );
    mDragMoves.setParent( registry );
    mDragMoves.setInitialFocus( MOVE );
    PathFinderController& controller = registry->getPathFinderController();
    QObject::connect( &controller, &PathFinderController::pathFound,  this, &MoveDragController::onPathFound,  Qt::DirectConnection );
    QObject::connect( &controller, &PathFinderController::testResult, this, &MoveDragController::onTestResult, Qt::DirectConnection );
    mTileDragTestResult.setParent(this);
}

void MoveDragController::onBoardLoaded( Board& board )
{
    MoveBaseController::onBoardLoaded( board );
    setDragState( Inactive );
}

void MoveDragController::dragStart( const ModelPoint& startPoint )
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        mChanged = false;
        mPreviousCoord = QPoint();

        if ( startPoint.equals( getBaseFocusVector() ) ) {
            setDragState( DraggingTank );
        } else {
            Board* board = registry->getGame().getBoard(true);
            if ( Piece* piece = board->getPieceManager().pieceAt( startPoint ) ) {
                if ( mTileDragTestCriteria.setTileDragCriteria( mFocus, piece, &mTileDragTestResult ) ) {
#ifndef QT_NO_DEBUG
                    if ( mDragState == Searching ) {
                        std::cout << "dragStart: already searching" << std::endl;
                    }
#endif // QT_NO_DEBUG
                    setDragState( Searching );
                    registry->getPathFinderController().testCriteria( &mTileDragTestCriteria );
                } else {
                    setDragState( Forbidden );
                }
            } else switch( board->tileAt( startPoint ) ) {
            case DIRT:
            case TILE_SUNK:
            case FLAG:
            {   PathSearchAction& pathToAction = registry->getPathToAction();
                pathToAction.setCriteria( mFocus, startPoint );
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

void MoveDragController::move( int direction )
{
    if ( mDragState == Inactive ) {
        MoveBaseController::move( direction );
    } else {
        if ( mFocus == TANK ) {
            undoMoves();
            setFocus( MOVE );
        }

        moveInternal( mDragMoves, direction );
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

void MoveDragController::undo()
{
    if ( mDragState == Inactive ) {
        MoveBaseController::undo();
    } else if ( mFocus == MOVE ) {
        undoLastMove();
    } else {
        undoMoves();
        setFocus( MOVE );
    }
}

void MoveDragController::setFocus( PieceType what )
{
    mDragMoves.replaceBack( what == TANK ? MOVE : MOVE_HIGHLIGHT );
    MoveBaseController::setFocus( what );
}

int MoveDragController::getPushIdDelineation() const
{
    return mMinPushedId;
}

void MoveDragController::fire( int count )
{
    if ( mDragState != Inactive ) {
        fireInternal( mDragMoves, count );
    } else {
        MoveBaseController::fire( count );
    }
}

int MoveDragController::getLastUsedPushId() const
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        return registry->getGame().getBoard(true)->getLastPushId();
    }
    return 0;
}

void MoveDragController::setDragState( DragState state )
{
    if ( mDragState != state ) {
        if ( mDragState == Inactive ) {
            mMinPushedId = getLastUsedPushId();
        } else if ( state == Inactive ) {
            mTileDragFocusAngle = -1;
            mTileDragAngleMask = 0;

            int lastMinPushedId = mMinPushedId;
            mMinPushedId = -1;
            if ( lastMinPushedId >= 0 && getLastUsedPushId() > mMinPushedId ) {
                // invalidate deltas:
                emit invalidatePushIdDelineation( lastMinPushedId );
            }

            if ( mDragMoves.size() ) {
                for( auto it : mDragMoves.getList() ) {
                    if ( int uid = it->getShotPathUID() ) {
                        for( const auto& fsit : mFutureShots.getPaths() ) {
                            if ( fsit.getUID() >= uid ) {
                                mFutureShots.invalidate( fsit );
                            }
                        }
                        break;
                    }
                }

                if ( mFocus == TANK ) {
                    mMoves.reset( &mDragMoves, false );
                    setFocus( MOVE );
                } else {
                    mMoves.replaceBack( MOVE );
                    mMoves.appendList( &mDragMoves, false );
                    mMoves.replaceBack( MOVE_HIGHLIGHT );
                }
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

ModelVector MoveDragController::getDragFocusVector( PieceType focus ) const
{
    if ( focus == NONE || focus == MOVE ) {
        if ( Piece* move = mDragMoves.getBack() ) {
            if ( mFocus == MOVE || move->getType() == MOVE_HIGHLIGHT ) {
                return *move;
            }
        }
    }
    return getBaseFocusVector( focus );
}

void MoveDragController::onPathFound( PieceListManager* path, PathSearchCriteria* criteria )
{
    if ( mDragState == Inactive ) {
        applyPathUsingCriteria( path, criteria );
    } else {
#ifndef QT_NO_DEBUG
        if ( mDragState != Searching ) {
            std::cout << "onPathFound: dragState=" << mDragState << std::endl;
        }
        if ( !criteria->getStartPoint().equals(getBaseFocusVector()) ) {
            std::cout << "onPathFound: start " << criteria->getStartCol() << "," << criteria->getStartRow() << " != focus" << std::endl;
        }
#endif // QT_NO_DEBUG
        mDragMoves.reset( path );
        mDragMoves.replaceBack( MOVE_HIGHLIGHT );
        if ( criteria->getCriteriaType() == PathSearchCriteria::TileDragTestCriteria ) {
            moveInternal( mDragMoves, mTileDragFocusAngle );
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
static int coordLeaning( QPoint coord, const ModelPoint& square )
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
        std::cout << "** MoveDragController::onDragTo null focus" << std::endl;
        setDragState( Inactive );
        return;
    }

    switch( mDragState ) {
    case Inactive:
    case Searching:
        return;

    case Forbidden:
    case ForbiddenTank:
    case DraggingTank:
        if ( p == focusVector ) {
            setDragState( DraggingTank );
        } else if ( GameRegistry* registry = getRegistry(this) ) {
            for( int angle = 0; angle < 360; angle += 90 ) {
                ModelPoint toPoint( focusVector );
                Piece* pushPiece = nullptr;
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
                                }
                                mDragMoves.replaceBack( MOVE_HIGHLIGHT );
                            }
                        } else {
                            moveInternal( mDragMoves, angle );
                            lastMove = mDragMoves.getBack();
                            if ( lastMove && !p.equals( *lastMove ) ) {
                                moveInternal( mDragMoves, angle );
                            }
                        }
                        mChanged = true;
                        return;
                    }
                }
            }
            if ( mDragState == DraggingTank ) {
                setDragState( ForbiddenTank );
            }
        }

        // check for rotation change:
    {   int rotation = coordLeaning( coord, focusVector );
        if ( rotation >= 0 && rotation != focusVector.mAngle && rotation != coordLeaning( mPreviousCoord, focusVector ) ) {
            moveInternal( mDragMoves, rotation );
        }
    }
        break;

    case ForbiddenTile:
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
                if ( !piece->getShotCount() && getBaseFocusVector().ModelPoint::equals( *piece ) ) {
                    mDragMoves.eraseBack();
                }
            }
        }
    }
    setDragState( Inactive );
    wakeup();
}

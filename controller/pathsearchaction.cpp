#include "pathsearchaction.h"
#include "pathfindercontroller.h"
#include "gameregistry.h"
#include "movecontroller.h"
#include "model/tank.h"


PathSearchAction::PathSearchAction( QObject*parent ) : QAction(parent)
{
}

bool PathSearchAction::setCriteria( PieceType focus, const ModelPoint& target, bool moveWhenFound )
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        mTargetPoint = target;
        mMoveWhenFound = moveWhenFound;

        MoveController& moveController = registry->getMoveController();

        if ( focus == TANK ) {
            mFocus = TANK;
        } else {
            Piece* move = moveController.getMoves()->getBack();
            if ( !move ) {
                mFocus = TANK;
            } else {
                mFocus = MOVE;
                mStartPoint = *move;
                mStartDirection = move->getAngle();
            }
        }

        if ( mFocus == TANK ) {
            mStartPoint = registry->getTank().getPoint();
            mStartDirection = registry->getTank().getRotation();
        }

        return true;
    }
    return false;
}

PathSearchCriteria *PathSearchAction::getCriteria()
{
    return dynamic_cast<PathSearchCriteria*>(this);
}

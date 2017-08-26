#include "pathsearchaction.h"
#include "pathfindercontroller.h"
#include "gameregistry.h"
#include "movecontroller.h"
#include "model/tank.h"


PathSearchAction::PathSearchAction( QObject*parent ) : QAction(parent)
{
}

bool PathSearchAction::setCriteria( PieceType focus, const ModelPoint& target )
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        mTargetPoint = target;

        MoveController& moveController = registry->getMoveController();

        if ( focus == TANK ) {
            mFocus = TANK;
        } else {
            Piece* move = moveController.getMoves().getBack();
            if ( !move ) {
                mFocus = TANK;
            } else {
                mFocus = MOVE;
                mStartVector = *move;
                mStartDirection = move->getAngle();
            }
        }

        if ( mFocus == TANK ) {
            mStartVector = registry->getTank().getVector();
            mStartDirection = registry->getTank().getRotation();
        }

        return true;
    }
    return false;
}

PathSearchCriteria* PathSearchAction::getCriteria()
{
    return dynamic_cast<PathSearchCriteria*>(this);
}

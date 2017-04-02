#include "pathsearchaction.h"
#include "pathfindercontroller.h"
#include "game.h"

PathSearchAction::PathSearchAction(QObject *parent) : QAction(parent)
{
}

void PathSearchAction::setCriteria( PieceType focus, const ModelPoint& target, bool moveWhenFound )
{
    if ( Game* game = getGame(this) ) {
        MoveController* moveController = game->getMoveController();

        if ( focus == TANK ) {
            mFocus = TANK;
        } else {
            Piece* move = moveController->getMoves()->getBack();
            if ( !move ) {
                mFocus = TANK;
            } else {
                mFocus = MOVE;
                mStartPoint = *move;
                mStartDirection = move->getAngle();
            }
        }

        if ( mFocus == TANK ) {
            Tank* tank = game->getTank();
            mStartPoint = tank->getPoint();
            mStartDirection = tank->getRotation();
        }
    }

    mTargetPoint = target;
    mMoveWhenFound = moveWhenFound;
}

PathSearchCriteria *PathSearchAction::getCriteria()
{
    return dynamic_cast<PathSearchCriteria*>(this);
}

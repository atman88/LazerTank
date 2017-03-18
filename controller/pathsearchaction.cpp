#include "pathsearchaction.h"
#include "pathfindercontroller.h"
#include "game.h"

PathSearchAction::PathSearchAction(QObject *parent) : QAction(parent)
{
}

void PathSearchAction::setCriteria( PieceType focus, int targetCol, int targetRow, bool moveWhenFound )
{
    if ( Game* game = getGame(this) ) {
        Tank* tank = game->getTank();

        if ( focus == TANK ) {
            mFocus = TANK;
        } else {
            Piece* move = tank->getMoves()->getBack();
            if ( !move ) {
                mFocus = TANK;
            } else {
                mFocus = MOVE;
                mStartCol = move->getCol();
                mStartRow = move->getRow();
                mStartDirection = move->getAngle();
            }
        }

        if ( mFocus == TANK ) {
            mStartCol = tank->getCol();
            mStartRow = tank->getRow();
            mStartDirection = tank->getRotation();
        }
    }

    mTargetCol = targetCol;
    mTargetRow = targetRow;
    mMoveWhenFound = moveWhenFound;
}

PathSearchCriteria *PathSearchAction::getCriteria()
{
    return dynamic_cast<PathSearchCriteria*>(this);
}

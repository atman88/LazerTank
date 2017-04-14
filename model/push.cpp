#include "push.h"
#include "controller/gameregistry.h"
#include "controller/game.h"

Push::Push( QObject* parent ) : PushView(parent)
{
}

void Push::start( Piece& what, int fromCol, int fromRow, int toCol, int toRow )
{
    mTargetCol = toCol;
    mTargetRow = toRow;
    PushView::start( what, fromCol*24, fromRow*24, toCol*24, toRow*24 );
}

int Push::getTargetCol() const
{
    return mTargetCol;
}

int Push::getTargetRow() const
{
    return mTargetRow;
}

void Push::stopping()
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        registry->getGame().onPushed( getType(), mTargetCol, mTargetRow, getPieceAngle() );
    }
}

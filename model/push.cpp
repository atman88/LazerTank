#include "push.h"
#include "controller/gameregistry.h"
#include "controller/game.h"

Push::Push( QObject* parent ) : PushView(parent)
{
}

void Push::start( Piece& what, ModelPoint fromPoint, ModelPoint toPoint )
{
    mTargetPoint = toPoint;
    PushView::start( what, fromPoint.mCol*24, fromPoint.mRow*24, toPoint.mCol*24, toPoint.mRow*24 );
}

ModelPoint Push::getTargetPoint() const
{
    return mTargetPoint;
}

bool Push::occupies( ModelPoint& square )
{
    return PushView::occupies( QPoint(square.mCol*24+24/2, square.mRow*24+24/2) );
}

void Push::stopping()
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        registry->getGame().onPushed( getType(), mTargetPoint, getPieceAngle() );
    }
}

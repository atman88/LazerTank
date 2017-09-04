#include "pathsearchaction.h"
#include "gameregistry.h"

PathSearchAction::PathSearchAction( QObject*parent ) : QAction(parent)
{
}

bool PathSearchAction::setCriteria( PieceType focus, const ModelPoint& target )
{
    return mCriteria.setPathCriteria( focus, target, this );
}

PathSearchCriteria& PathSearchAction::getCriteria()
{
    return mCriteria;
}

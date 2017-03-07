#include <QVariant>

#include "gameutils.h"

// find the game from the object hierarchy:

Game* getGame( QObject* gameObject )
{
    QObject* p = gameObject;
    QVariant v;

    while( p && !(v = p->property("GameHandle")).isValid() ) {
        p = p->parent();
    }

    return v.value<GameHandle>().game;
}

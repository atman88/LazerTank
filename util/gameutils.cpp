#include <QVariant>
#include <QPoint>

#include "gameutils.h"
#include "controller/gameregistry.h"

const char* GameHandleName = "GameHandle";

// find the registry from the object hierarchy:

GameRegistry* getRegistry( const QObject* gameObject )
{
    const QObject* p = gameObject;
    QVariant v;

    while( p && !(v = p->property(GameHandleName)).isValid() ) {
        p = p->parent();
    }

    return v.value<GameHandle>().registry;
}

void centerToEntryPoint( int angle, QPoint *point )
{
    switch( angle ) {
    case   0: point->setY( point->y()+24/2 ); break;
    case  90: point->setX( point->x()-24/2 ); break;
    case 180: point->setY( point->y()-24/2 ); break;
    case 270: point->setX( point->x()+24/2 ); break;
    }
}

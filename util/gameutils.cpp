#include <QVariant>
#include <QPoint>

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

QPoint modelToViewCenterSquare( int col, int row )
{
    return QPoint( col*24+24/2, row*24+24/2 );
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

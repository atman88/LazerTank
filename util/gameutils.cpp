#include <QVariant>
#include <QPoint>
#include <QApplication>
#include <QDesktopWidget>
#include <QMessageBox>

#include "gameutils.h"
#include "controller/gameregistry.h"
#include "controller/movecontroller.h"
#include "model/tank.h"

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

int myScreenHeight()
{
    return QApplication::desktop()->availableGeometry().height();
}

int myScreenWidth()
{
    return QApplication::desktop()->availableGeometry().width();
}

int checkForReplay( GameRegistry* registry )
{
    MoveController& moveController = registry->getMoveController();
    if ( moveController.replaying() ) {
        registry->getTank().pause();

        QMessageBox::StandardButton button = QMessageBox::question( nullptr, "Auto Replay", "Play from here?",
                                                                    QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes );
        if ( button == QMessageBox::Yes ) {
            moveController.setReplay( false );
        }
        if ( moveController.canWakeup() ) {
            registry->getTank().resume();
        }

        return (button == QMessageBox::Yes) ? -1 /* indicate changed to inactive */ : 1 /* indicate replay is active */;
    }

    return 0; // indicate replay inactive
}

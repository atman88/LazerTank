#include <QVariant>
#include "gameregistry.h"
#include "view/boardwindow.h"


GameRegistry::GameRegistry( Game& game, BoardWindow* window, WorkerThread& worker ) : QObject(0), mGame(game), mWindow(window),
  mWorker(worker)
{
    mHandle.registry = this;
    setProperty("GameHandle", QVariant::fromValue(mHandle));

    mCaptureAction.setParent(this);
    mPathToAction.setParent(this);

    if ( window ) {
        QObject::connect( window, &BoardWindow::destroyed, this, &GameRegistry::onWindowDestroyed );
    }
}

GameRegistry::~GameRegistry()
{
    // for injection support, disassociate the children of this class (also inhibits deleting)
    const QObjectList& children = QObject::children();
    while( !children.empty() ) {
        children.back()->setParent(0);
    }
}

void GameRegistry::onWindowDestroyed()
{
    mWindow = 0;
}

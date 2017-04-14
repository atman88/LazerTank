#include "gameinitializer.h"
#include "gameregistry.h"
#include "game.h"

GameInitializer::GameInitializer() : QObject(0)
{
}

void GameInitializer::init( GameRegistry& registry )
{
    setParent( &registry );
    if ( BoardWindow* window = registry.getWindow() ) {
        QObject::connect( window, &BoardWindow::paintable, this, &GameInitializer::initGame, Qt::QueuedConnection );
        window->setVisible(true);
    }
}

void GameInitializer::initGame()
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        // do this on the app thread:
        registry->getGame().init( registry );
        // load the board via a background thread:
        registry->getWorker().doWork( this );
    }
}

void GameInitializer::run()
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        registry->getGame().onBoardLoading();
        registry->getGame().getBoard()->load( 1 );
    }
}

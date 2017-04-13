#include "gameinitializer.h"
#include "gameregistry.h"
#include "game.h"

GameInitializer::GameInitializer() : QObject(0)
{
}

void GameInitializer::init( GameRegistry& registry )
{
    setParent( &registry );
    if ( BoardWindow* window = registry.mWindow ) {
        QObject::connect( window, &BoardWindow::paintable, this, &GameInitializer::initGame, Qt::QueuedConnection );
        window->setVisible(true);
    }
}

void GameInitializer::initGame()
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        // do this on the app thread:
        registry->mGame.init( registry );
        // load the board via a background thread:
        registry->mWorker.doWork( this );
    }
}

void GameInitializer::run()
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        registry->mGame.onBoardLoading();
        registry->mGame.getBoard()->load( 1 );
    }
}

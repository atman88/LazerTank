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
        registry->getGame().init( registry );
    }
}

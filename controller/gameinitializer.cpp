#include "gameinitializer.h"
#include "gameregistry.h"
#include "game.h"
#include "model/level.h"

GameInitializer::GameInitializer() : QObject(0), mWindowPaintable(false), mLevelListInitialized(false)
{
}

void GameInitializer::init( GameRegistry& registry )
{
    setParent( &registry );

    //
    // first phase: parallel execution of
    //   1.1) window bring up
    //   1.2) background level list initialization
    //
    if ( BoardWindow* window = registry.getWindow() ) {
        QObject::connect( window, &BoardWindow::paintable, this, &GameInitializer::onWindowPaintable, Qt::QueuedConnection );
        window->setVisible(true);
    }
    QObject::connect( &registry.getLevelList(), &LevelList::initialized, this, &GameInitializer::onLevelListInitialized );
    registry.getLevelList().init( &registry );
}

void GameInitializer::onWindowPaintable()
{
    mWindowPaintable = true;
    resumeInitialization();
}

void GameInitializer::onLevelListInitialized()
{
    mLevelListInitialized = true;
    resumeInitialization();
}

void GameInitializer::resumeInitialization()
{
    //
    // second phase: sequential execution of:
    //   2.1) initialization of the game on the foreground thread
    //   2.2) kick off loading the board first board
    //
    if ( mWindowPaintable && mLevelListInitialized ) {
        if ( GameRegistry* registry = getRegistry(this) ) {
            registry->getGame().init( registry );
            registry->getGame().getBoard()->load( registry->getLevelList().nextLevel(-1) );
        }
    }
}

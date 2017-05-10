#include <iostream>
#include "gameinitializer.h"
#include "gameregistry.h"
#include "game.h"
#include "model/level.h"

GameInitializer::GameInitializer() : QObject(0), mInitPhase(PendingPhase), mWindowPaintable(false)
{
}

void GameInitializer::init( GameRegistry& registry )
{
    setParent( &registry );

    if ( mInitPhase != PendingPhase ) {
        std::cout << "GameInitializer::init out of phase! " << mInitPhase << "!=" << PendingPhase << std::endl;
        return;
    }
    mInitPhase = ChooserPhase;

    //
    // first phase: parallel execution of
    //   1.1) window bring up
    //   1.2) background level list initialization
    //
    if ( BoardWindow* window = registry.getWindow() ) {
        QObject::connect( window, &BoardWindow::paintable, this, &GameInitializer::onWindowPaintable, Qt::QueuedConnection );
        window->setVisible(true);
    }
    QObject::connect( &registry.getLevelChooser(), &LevelChooser::listInitialized, this, &GameInitializer::resume );
    registry.getLevelChooser().init( &registry );
}

void GameInitializer::onWindowPaintable()
{
    if ( !mWindowPaintable ) {
        mWindowPaintable = true;
        resume();
    }
}

void GameInitializer::resume()
{
    // currently the ChooserPhase is the only defined phase to resume from
    if ( mInitPhase != ChooserPhase ) {
        return;
    }

    //
    // second phase: sequential execution of:
    //   2.1) initialization of the game & window on the foreground thread
    //   2.2) kick off loading the first board
    //
    if ( mWindowPaintable ) {
        if ( GameRegistry* registry = getRegistry(this) ) {
            if ( registry->getLevelChooser().isListInitialized() ) {
                mInitPhase = GamePhase;

                registry->getGame().init( registry );
                if ( BoardWindow* window = registry->getWindow() ) {
                    window->init( registry );
                }
                registry->getGame().loadMasterBoard( registry->getLevelChooser().nextLevel(0) );
            }
        }
    }
}

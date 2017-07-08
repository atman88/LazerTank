#include <iostream>
#include "gameinitializer.h"
#include "gameregistry.h"
#include "game.h"
#include "model/level.h"
#include "model/boardpool.h"


GameInitializer::GameInitializer() : QObject(0), mInitPhase(PendingPhase)
{
}

void GameInitializer::init( GameRegistry& registry )
{
    setParent( &registry );

    if ( mInitPhase != PendingPhase ) {
        std::cout << "GameInitializer::init out of phase! " << mInitPhase << "!=" << PendingPhase << std::endl;
        return;
    }
    mInitPhase = LevelsPhase;

    //
    // first phase:
    //   background level list initialization
    //
    QObject::connect( &registry.getLevelList(), &LevelList::initialized, this, &GameInitializer::resume );
    registry.getLevelList().init( &registry );
}

void GameInitializer::resume()
{
    if ( GameRegistry* registry = getRegistry(this) ) {

        if ( mInitPhase == LevelsPhase ) {
            if ( registry->getLevelList().isInitialized()  ) {
                //
                // second phase:
                //    2.1) window bringup
                //    2.2) initialize the board pool
                if ( BoardWindow* window = registry->getWindow() ) {
                    QObject::connect( window, &BoardWindow::paintable, this, &GameInitializer::resume, Qt::QueuedConnection );
                    window->setVisible(true);

                    if ( QScreen* screen = window->screen() ) {
                        registry->getBoardPool().init( registry->getLevelList(), screen->availableSize().height() / 12 ); // chooser's TILE_SIZE
                    }
                }

                mInitPhase = WindowPhase;
            }
        }

        if ( mInitPhase == WindowPhase ) {
            if ( BoardWindow* window = registry->getWindow() ) {
                if ( window->isPaintable() ) {
                    //
                    // third phase: sequential execution of:
                    //   2.1) initialization of the game & window on the foreground thread
                    //   2.2) kick off loading the first board
                    //
                    mInitPhase = GamePhase;

                    registry->getGame().init( registry );
                    if ( BoardWindow* window = registry->getWindow() ) {
                        window->init( registry );
                    }
                    registry->getGame().loadMasterBoard( registry->getLevelList().nextLevel(0) );
                }
            }
        }
    }
}

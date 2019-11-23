#include <iostream>
#include "gameinitializer.h"
#include "gameregistry.h"
#include "game.h"
#include "model/level.h"
#include "model/boardpool.h"
#include "util/persist.h"
#include "view/levelchooser.h"

GameInitializer::GameInitializer() : QObject(nullptr), mInitPhase(PendingPhase)
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
//                    QObject::connect( window, &BoardWindow::paintable, this, &GameInitializer::resume, Qt::QueuedConnection );
                    window->setVisible(true);

                    registry->getBoardPool().init( registry->getLevelList(), myScreenHeight() / ChooserTileSize );
                }

                mInitPhase = WindowPhase;
            }
//            return;
        }

        Game& game = registry->getGame();

        if ( mInitPhase == WindowPhase ) {
            if ( BoardWindow* window = registry->getWindow() ) {
//                if ( window->isPaintable() ) {
                    //
                    // third phase: sequential execution of:
                    //   2.1) initialization of the game & window on the foreground thread
                    //   2.2) kick off loading the first board
                    //
                    mInitPhase = GamePhase;

                    game.init( registry );
                    window->init( registry );
                    QObject::connect( window, &BoardWindow::backdoor, &registry->getRecorder(), &Recorder::backdoor, Qt::QueuedConnection );
                    QObject::connect( &game, &Game::boardLoaded, this, &GameInitializer::resume, Qt::QueuedConnection );
                    game.loadMasterBoard( registry->getLevelList().nextLevel(0) );
//                }
            }
            return;
        }

        if ( mInitPhase == GamePhase ) {
            //
            // forth phase:
            //   initialize persistent storage
            mInitPhase = PersistPhase;

            QObject::disconnect( &game, &Game::boardLoaded, this, &GameInitializer::resume );

            Persist& persist = registry->getPersist();
            QObject::connect( &persist, &Persist::levelSetComplete, &registry->getLevelList(), &LevelList::setCompleted );
            persist.init( registry );
        }
    }
}

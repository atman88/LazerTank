#ifndef GAMEINITIALIZER_H
#define GAMEINITIALIZER_H

#include <QObject>
#include "view/boardwindow.h"
#include "controller/game.h"
#include "util/workerthread.h"

/**
 * @brief Controller class for initializing the game
 */
class GameInitializer : public QObject
{
    Q_OBJECT

    typedef enum {
        PendingPhase,
        LevelsPhase,
        WindowPhase,
        GamePhase,
        PersistPhase
    } InitPhase;

public:
    explicit GameInitializer();
    void init( GameRegistry& registry );

private slots:
    /**
     * @brief Executes game initalization (2nd phase)
     */
    void resume();

private:
    InitPhase mInitPhase;
};

#endif // GAMEINITIALIZER_H

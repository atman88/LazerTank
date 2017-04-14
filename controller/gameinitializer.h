#ifndef GAMEINITIALIZER_H
#define GAMEINITIALIZER_H

#include <QObject>
#include "view/boardwindow.h"
#include "controller/game.h"
#include "util/workerthread.h"

/**
 * @brief Controller class for initializing the game
 */
class GameInitializer : public QObject, Runnable
{
    Q_OBJECT

public:
    explicit GameInitializer();
    void init( GameRegistry& registry );

    // Implementation of the LoadRunnable
    void run() override;

private slots:
    /**
     * @brief Executes game initalization
     */
    void initGame();
};

#endif // GAMEINITIALIZER_H
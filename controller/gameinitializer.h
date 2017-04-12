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
    explicit GameInitializer(QObject* parent = 0);
    void init();

    // Implementation of the LoadRunnable
    void run() override;

private slots:
    /**
     * @brief Executes game initalization
     */
    void initGame();

public:
    BoardWindow mWindow;
    Game mGame;
    WorkerThread mWorker;
};

#endif // GAMEINITIALIZER_H

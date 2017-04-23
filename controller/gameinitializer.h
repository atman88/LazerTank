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

public:
    explicit GameInitializer();
    void init( GameRegistry& registry );

private slots:
    /**
     * @brief Receives notification that the window has begun rendering
     */
    void onWindowPaintable();

    /**
     * @brief Receives notification that the levels are initialized
     */
    void onLevelListInitialized();

    /**
     * @brief Executes game initalization
     */
    void resumeInitialization();

private:
    bool mWindowPaintable;
    bool mLevelListInitialized;
};

#endif // GAMEINITIALIZER_H

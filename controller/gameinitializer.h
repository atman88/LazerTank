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
        ChooserPhase,
        GamePhase
    } InitPhase;

public:
    explicit GameInitializer();
    void init( GameRegistry& registry );

private slots:
    /**
     * @brief Receives notification that the window has begun rendering
     */
    void onWindowPaintable();

    /**
     * @brief Executes game initalization (2nd phase)
     */
    void resume();

private:
    InitPhase mInitPhase;
    bool mWindowPaintable;
};

#endif // GAMEINITIALIZER_H

#include <QApplication>
#include "controller/gameinitializer.h"
#include "controller/gameregistry.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qRegisterMetaType<GameHandle>("GameHandle");

    Game game;
    WorkerThread worker;
    BoardWindow window;
    GameRegistry registry( game, &window, worker );
    GameInitializer initializer;
    initializer.init( registry );

    return app.exec();
}

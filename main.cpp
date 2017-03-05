#include <iostream>
#include <QApplication>
#include "view/BoardWindow.h"
#include "controller/game.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qRegisterMetaType<GameHandle>("GameHandle");

    BoardWindow window;
    Game game;
    game.init( &window );
    window.show();

    return app.exec();
}

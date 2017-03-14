#include <iostream>
#include <QApplication>
#include "view/boardwindow.h"
#include "controller/game.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qRegisterMetaType<GameHandle>("GameHandle");

    BoardWindow window;
    Game game;
    game.init( &window );
    game.getBoard()->load( 1 );
    window.show();

    return app.exec();
}

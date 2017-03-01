#include <iostream>
#include <QApplication>
#include "view/BoardWindow.h"
#include "controller/Game.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qRegisterMetaType<GameHandle>("GameHandle");

    BoardWindow window;
    Board board;
    Game game( &board );
    game.init( &window );
    board.load( 1 );
    window.show();

    return app.exec();
}

#include <iostream>
#include <QApplication>
#include "view/BoardWindow.h"
#include "controller/Game.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qRegisterMetaType<GameHandle>("GameHandle");
    qRegisterMetaType<PieceList>("MoveList");
    qRegisterMetaType<PieceList>("tiles");

    BoardWindow window;
    Board board(":/maps/default.txt");
    Game game( &board );
    window.setGame( game.getHandle() );
    window.show();

    return app.exec();
}

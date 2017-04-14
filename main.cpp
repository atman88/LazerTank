#include <QApplication>
#include "controller/gameinitializer.h"
#include "controller/gameregistry.h"


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qRegisterMetaType<GameHandle>("GameHandle");

    BoardWindow window;
    GameRegistry registry( &window );
    GameInitializer initializer;
    initializer.init( registry );

    return app.exec();
}

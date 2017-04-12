#include <QApplication>
#include "controller/gameinitializer.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qRegisterMetaType<GameHandle>("GameHandle");

    GameInitializer initializer;
    initializer.init();

    return app.exec();
}

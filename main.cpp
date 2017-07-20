#include <qglobal.h>
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
#include <cstdlib>
#endif // QT_VERSION

#include <QApplication>
#include "controller/gameinitializer.h"
#include "controller/gameregistry.h"


int main(int argc, char *argv[])
{
//#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
//    // work around for QTBUG-60264:
//    if ( char* imModule = getenv( "QT_IM_MODULE" ) ) {
//        if ( !strcmp( imModule, "ibus") ) {
//            setenv( "QT_IM_MODULE","compose", 1 );
//        }
//    }
//#endif // QT_VERSION

    QApplication app(argc, argv);

    qRegisterMetaType<GameHandle>( GameHandleName );

    BoardWindow window;
    GameRegistry registry( &window );
    GameInitializer initializer;
    initializer.init( registry );

    return app.exec();
}

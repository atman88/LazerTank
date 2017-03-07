######################################################################
# Automatically generated by qmake (3.0) Fri Feb 10 13:22:09 2017
######################################################################

TEMPLATE = app
INCLUDEPATH += . view controller model
QT += widgets

QMAKE_CXXFLAGS += -std=gnu++11

# Input
HEADERS += \
           model/board.h \
           model/piece.h \
    view/push.h \
    view/shooter.h \
    controller/pathfinder.h \
    util/imageutils.h \
    model/piecesetmanager.h \
    model/piecelistmanager.h \
    controller/speedcontroller.h \
    model/shotmodel.h \
    view/shotview.h \
    util/renderutils.h \
    controller/animationstateaggregator.h \
    controller/game.h \
    model/boarddelta.h \
    view/boardwindow.h \
    model/tank.h

SOURCES += \
           model/board.cpp \
           model/piece.cpp \
    view/push.cpp \
    view/shooter.cpp \
    controller/pathfinder.cpp \
    util/imageutils.cpp \
    model/piecesetmanager.cpp \
    model/piecelistmanager.cpp \
    controller/speedcontroller.cpp \
    model/shotmodel.cpp \
    view/shotview.cpp \
    util/renderutils.cpp \
    controller/animationstateaggregator.cpp \
    controller/game.cpp \
    model/boarddelta.cpp \
    view/boardwindow.cpp \
    model/tank.cpp

RESOURCES += qml.qrc

test{
    INCLUDEPATH += test/controller

    HEADERS += test/testmain.h

    TARGET = qlttest

    QT += testlib

    SOURCES += test/testmain.cpp \
        test/controller/testgame.cpp \
        test/model/testpiecelistmanager.cpp
}
else{
    TARGET = qlt

    SOURCES += main.cpp
}

DISTFILES += \
    maps/testcannon.txt

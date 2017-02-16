######################################################################
# Automatically generated by qmake (3.0) Fri Feb 10 13:22:09 2017
######################################################################

TEMPLATE = app
INCLUDEPATH += . view controller model test/controller
QT += widgets

QMAKE_CXXFLAGS += -std=gnu++11

# Input
HEADERS += controller/Game.h \
           model/board.h \
           model/piece.h \
           view/BoardWindow.h \
           view/tank.h \
    view/shot.h \
    view/push.h \
    controller/animationaggregator.h
SOURCES += controller/Game.cpp \
           view/BoardWindow.cpp \
           view/tank.cpp \
           model/board.cpp \
           model/piece.cpp \
    view/shot.cpp \
    view/push.cpp \
    controller/animationaggregator.cpp
RESOURCES += qml.qrc

test{
    TARGET = qlttest

    QT += testlib

    SOURCES += test/controller/testgame.cpp
}
else{
    TARGET = qlt

    SOURCES += main.cpp
}

######################################################################
# Automatically generated by qmake (3.0) Fri Feb 10 13:22:09 2017
######################################################################

TEMPLATE = app
INCLUDEPATH += . view controller model
QT += widgets

QMAKE_CXXFLAGS += -std=gnu++11

HEADERS += \
    model/board.h \
    model/piece.h \
    model/piecesetmanager.h \
    model/piecelistmanager.h \
    model/modelpoint.h \
    view/pieceview.h \
    util/gameutils.h \
    util/imageutils.h \
    view/boardrenderer.h

SOURCES += \
    model/board.cpp \
    model/piece.cpp \
    model/piecesetmanager.cpp \
    model/piecelistmanager.cpp \
    model/modelpoint.cpp \
    view/pieceview.cpp \
    util/gameutils.cpp \
    util/imageutils.cpp \
    view/boardrenderer.cpp

index {
    TARGET = qltindexer
    SOURCES +=  index/indexermain.cpp
}
else {
# definitions common to app & test

QT += xml

RESOURCES += qml.qrc

HEADERS += \
    view/shooter.h \
    controller/pathfinder.h \
    controller/speedcontroller.h \
    model/shotmodel.h \
    view/shotview.h \
    controller/animationstateaggregator.h \
    controller/game.h \
    model/boarddelta.h \
    view/boardwindow.h \
    model/tank.h \
    view/tankview.h \
    controller/pathsearchaction.h \
    controller/pathfindercontroller.h \
    controller/pathsearchcriteria.h \
    model/futureshotpath.h \
    view/pushview.h \
    model/push.h \
    controller/futurechange.h \
    model/tile.h \
    controller/movecontroller.h \
    util/recorder.h \
    util/recorderprivate.h \
    view/replaytext.h \
    util/workerthread.h \
    controller/gameinitializer.h \
    controller/gameregistry.h \
    model/level.h \
    view/levelchooser.h \
    model/boardpool.h

SOURCES += \
    view/shooter.cpp \
    controller/pathfinder.cpp \
    controller/speedcontroller.cpp \
    model/shotmodel.cpp \
    view/shotview.cpp \
    controller/animationstateaggregator.cpp \
    controller/game.cpp \
    model/boarddelta.cpp \
    view/boardwindow.cpp \
    model/tank.cpp \
    view/tankview.cpp \
    controller/pathsearchaction.cpp \
    controller/pathfindercontroller.cpp \
    controller/pathsearchcriteria.cpp \
    model/futureshotpath.cpp \
    view/pushview.cpp \
    model/push.cpp \
    controller/movecontroller.cpp \
    util/recorder.cpp \
    util/recorderprivate.cpp \
    view/replaytext.cpp \
    util/workerthread.cpp \
    controller/gameinitializer.cpp \
    controller/gameregistry.cpp \
    model/level.cpp \
    view/levelchooser.cpp \
    model/boardpool.cpp

test{
    INCLUDEPATH += test/controller

    HEADERS += test/testmain.h \
      test/util/piecelistmanagerobserver.h

    TARGET = qlttest

    QT += testlib

    SOURCES += test/testmain.cpp \
        test/controller/testgame.cpp \
        test/model/testpiecelistmanager.cpp \
        test/model/testfutureshotpath.cpp \
        test/controller/testmovecontroller.cpp \
        test/util/testrecorder.cpp \
        test/util/piecelistmanagerobserver.cpp \
        test/util/testworker.cpp \
        test/model/testboardpool.cpp

} else {
    TARGET = qlt

    SOURCES += main.cpp

    levelindex.target = maps/levels.xml
    levelindex.commands = qltindexer > $$levelindex.target
    levelindex.depends = maps/*.txt
    QMAKE_EXTRA_TARGETS += levelindex

    TARGET.depends += levelindex
}
}

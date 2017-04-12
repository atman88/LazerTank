#include "gameinitializer.h"

GameInitializer::GameInitializer(QObject *parent) : QObject(parent)
{
}

void GameInitializer::init()
{
    QObject::connect( &mWindow, &BoardWindow::paintable, this, &GameInitializer::initGame, Qt::QueuedConnection );
    mWindow.setVisible(true);
}

void GameInitializer::initGame()
{
    // do this on the app thread:
    mGame.init( &mWindow );
    // load the board via a background thread:
    mWorker.doWork( this );
}

void GameInitializer::run()
{
    mGame.onBoardLoading();
    mGame.getBoard()->load( 1 );
}

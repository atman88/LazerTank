#include <QObject>
#include <QPushButton>
#include "levelcompleteddialog.h"
#include "controller/gameregistry.h"
#include "controller/game.h"
#include "model/board.h"
#include "model/level.h"
#include "util/recorder.h"

LevelCompletedDialog::LevelCompletedDialog( GameRegistry* registry ) : mRegistry(registry)
{
    setWindowTitle( "Level completed!" );
    setText( QString( "Completed in %1 moves" ).arg( registry->getRecorder().getRecordedCount() ) );
    setIconPixmap( QPixmap(":/images/flag.png") );
    addButton( QString("&Auto Replay" ), ActionRole );
    QPushButton* nextButton = addButton( QString("&Next Level" ), AcceptRole );
    addButton( QString("E&xit" ), DestructiveRole );
    setDefaultButton( nextButton );
    int number = registry->getGame().getBoard()->getLevel();
    nextButton->setEnabled( registry->getLevelList().nextLevel( number ) );

    setLevelCountInternal();
    QObject::connect( &registry->getLevelList(), &LevelList::levelUpdated, this, &LevelCompletedDialog::onLevelUpdated );
}

LevelCompletedDialog::ButtonCode LevelCompletedDialog::getClickedCode() const
{
    if ( QAbstractButton* button = clickedButton() ) {
        switch( buttonRole( button ) ) {
        case ActionRole:      return Replay;
        case AcceptRole:      return Next;
        case DestructiveRole: return Exit;
        default:
            ;
        }
    }
    return None;
}

void LevelCompletedDialog::onLevelUpdated( const QModelIndex& )
{
    setLevelCountInternal();
}

void LevelCompletedDialog::keyPressEvent( QKeyEvent* e )
{
    if ( !e->isAutoRepeat() ) {
        switch( e->key() ) {
        case Qt::Key_D: // dump recording (debug)
            if ( e->modifiers() == Qt::AltModifier ) {
                mRegistry->getRecorder().dump();
            }
            break;
        }
    }
}

void LevelCompletedDialog::setLevelCountInternal()
{
    setInformativeText( QString( "%1 levels completed" ).arg( mRegistry->getLevelList().getCompletedCount() ) );
}


#include <QMouseEvent>
#include <QWhatsThis>

#include "whatsthisaware.h"
#include "boardwindow.h"
#include "controller/gameregistry.h"

WhatsThisAwareLabel::WhatsThisAwareLabel( QWidget* parent ) : QLabel(parent)
{
    mAction.setText( "What's this?" );
}

void WhatsThisAwareLabel::mousePressEvent( QMouseEvent* event )
{
    if ( event->button() == Qt::RightButton ) {
        if ( GameRegistry* registry = getRegistry(this) ) {
            QPoint globalPos = event->globalPos();
            QAction* action = registry->getWindow()->showMenu( &globalPos, { &mAction } );
            if ( action == &mAction ) {
                QWhatsThis::showText( globalPos, whatsThis() );
            }
        }
    }
}

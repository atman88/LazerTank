
#include <QWhatsThis>

#include "helputils.h"
#include "imageutils.h"

void whatsthis( QPoint* pos, unsigned what )
{
    if ( pos ) {
        const ResourcePixmap* pixmap = ResourcePixmap::getPixmap( what );
        QWhatsThis::showText( *pos, pixmap->getName() );
    }
}

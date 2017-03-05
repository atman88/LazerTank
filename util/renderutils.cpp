#include <iostream>
#include "renderutils.h"
#include "imageutils.h"

void renderRotation( int x, int y, int angle, QPainter* painter )
{
    int centerX = x + 24/2;
    int centerY = y + 24/2;
    painter->translate(centerX, centerY);
    painter->rotate(angle);
    painter->translate(-centerX, -centerY);
}

void renderRotatedPixmap( const QPixmap* pixmap, int x, int y, int angle, QPainter* painter )
{
    if ( angle ) {
        renderRotation( x, y, angle, painter );
    }
    painter->drawPixmap( x, y, *pixmap );
    painter->resetTransform();
}

void renderPiece( PieceType type, int x, int y, int angle, QPainter* painter )
{
    const QPixmap* pixmap = getPixmap( type );
    if ( pixmap->isNull() ) {
        std::cout << "no pixmap for " << type << std::endl;
        return;
    }

    renderRotatedPixmap( pixmap, x, y, angle, painter );
}

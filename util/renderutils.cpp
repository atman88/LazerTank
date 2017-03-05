#include "renderutils.h"

void renderRotation( int x, int y, int angle, QPainter* painter )
{
    int centerX = x + 24/2;
    int centerY = y + 24/2;
    painter->translate(centerX, centerY);
    painter->rotate(angle);
    painter->translate(-centerX, -centerY);
}


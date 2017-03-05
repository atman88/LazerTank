#ifndef RENDERUTILS_H
#define RENDERUTILS_H

#include <QPainter>
#include "model/piece.h"

extern void renderRotation( int x, int y, int angle, QPainter* painter );
extern void renderRotatedPixmap(const QPixmap* pixmap, int x, int y, int angle, QPainter* painter );
extern void renderPiece(PieceType type, int x, int y, int angle, QPainter* painter );

#endif // RENDERUTILS_H

#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H

#include <QPixmap>
#include <QPainter>

extern const QPixmap *getPixmap( unsigned type );
extern void drawPixmap(int x, int y, unsigned type , QPainter* painter);

#endif // IMAGEUTILS_H

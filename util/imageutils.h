#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H

#include <QPixmap>
#include <QPainter>

/**
 * @brief Retrieve the image for the given piece or tile type
 * @param type The PieceType or TileType to retrieve
 * @return The associated pixmap. An empty pixmap is returned for unregistered types.
 */
extern const QPixmap *getPixmap( unsigned type );

/**
 * @brief Paint the associated pixmap
 * @param x The upperleft x coordinate to paint at
 * @param y The upperleft y coordinate to paint at
 * @param type The pixmap type
 * @param painter The painter used to paint
 */
extern void drawPixmap(int x, int y, unsigned type , QPainter* painter);

#endif // IMAGEUTILS_H

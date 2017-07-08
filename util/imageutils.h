#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H

#include <QtGlobal>
QT_FORWARD_DECLARE_CLASS(QPixmap)

/**
 * @brief Retrieve the image for the given piece or tile type
 * @param type The PieceType or TileType to retrieve
 * @return The associated pixmap. An empty pixmap is returned for unregistered types.
 */
extern const QPixmap *getPixmap( unsigned type );

#endif // IMAGEUTILS_H

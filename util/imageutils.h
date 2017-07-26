#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H

#include <QtGlobal>
QT_FORWARD_DECLARE_CLASS(QPixmap)

#include "view/pieceview.h"

typedef enum {
    TANK_FAST = PieceTypeUpperBound,
    PixmapTypeUpperBound // must be last
} PixmapType;

/**
 * @brief Retrieve the image for the given piece or tile type
 * @param type Identifier. Can be a PieceType, TileType or PixmapType value.
 * @return The associated pixmap. An empty pixmap is returned for unregistered types.
 */
extern const QPixmap* getPixmap( unsigned type );

#endif // IMAGEUTILS_H

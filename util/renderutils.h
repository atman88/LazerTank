#ifndef RENDERUTILS_H
#define RENDERUTILS_H

#include <QPainter>
#include "model/piece.h"

/**
 * @brief Apply the transformation for for rendering the given position with the given rotation
 * @param x Upperleft x window coordinate
 * @param y Upperleft y window coordinate
 * @param angle The rotation
 * @param painter The painter to apply to
 */
extern void renderRotation( int x, int y, int angle, QPainter* painter );

/**
 * @brief Paint the pixmap at the given location with the given rotation
 * @param pixmap The image to paint
 * @param x Upperleft x window coordinate
 * @param y Upperleft y window coordinate
 * @param angle The rotation
 * @param painter The painter to apply to
 */
extern void renderRotatedPixmap(const QPixmap* pixmap, int x, int y, int angle, QPainter* painter );

/**
 * @brief Paint the piece type at the given location with the given rotation
 * @param type The type associated with the desired image
 * @param x Upperleft x window coordinate
 * @param y Upperleft y window coordinate
 * @param angle The rotation
 * @param painter The painter to apply to
 */
extern void renderPiece(PieceType type, int x, int y, int angle, QPainter* painter );

#endif // RENDERUTILS_H

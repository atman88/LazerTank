#ifndef BOARDRENDERER_H
#define BOARDRENDERER_H

#include <QPoint>
#include "model/modelpoint.h"
#include "model/piece.h"

QT_FORWARD_DECLARE_CLASS(QRect)
QT_FORWARD_DECLARE_CLASS(QPainter)

class Board;
class GameRegistry;

/**
 * @brief A light-weight helper class for rendering boards for a specific tile size
 */
class BoardRenderer
{
public:
    explicit BoardRenderer( int tileSize );

    int tileSize() const;

    void render( const QRect& rect, Board* board, QPainter* painter ) const;
    void renderInitialTank( Board* board, QPainter* painter );

    /**
     * @brief Gets the rectangular screen area for the given model point
     * @param forPoint The point to obtain the area of
     * @param rect The rect to return the result in
     * @return the rect parameter
     */
    QRect* getBounds( const ModelPoint& forPoint, QRect *rect ) const;

    /**
     * @brief Paint the associated pixmap
     * @param square The bounding rectangle of the board position to paint in
     * @param type The pixmap identifier
     * @param painter The painter used to paint
     */
    static void renderPixmap( QRect& square, unsigned type, QPainter* painter );

    /**
     * @brief Apply a transformation for rendering the given position with the given angle
     * @param square The bounding rectangle of the board position to transform to
     * @param angle The rotation
     * @param painter The painter to apply to
     */
    static void renderRotation( QRect& square, int angle, QPainter* painter );

    /**
     * @brief Paint the pixmap at the given location with the given rotation
     * @param pixmap The image to paint
     * @param square The bounding rectangle of the board position to transform to
     * @param angle The rotation
     * @param painter The painter to apply to
     */
    static void renderRotatedPixmap( const QPixmap* pixmap, QRect& square, int angle, QPainter* painter );

    /**
     * @brief Paint the piece type at the given location with the given rotation
     * @param type The type associated with the desired image
     * @param square The bounding rectangle of the board position to transform to
     * @param angle The rotation
     * @param painter The painter to apply to
     */
    static void renderPiece( PieceType type, QRect& square, int angle, QPainter* painter );

    /**
     * @brief Helper method to render the given set within the given rectangular area
     * @param iterator The starting position within the set
     * @param end The end of set position
     * @param dirty The rectangular area being rendered
     * @param painter The painter associated with this render operation
     */
    void renderListIn( PieceSet::iterator iterator, PieceSet::iterator end, const QRect* dirty, QPainter* painter );

    void renderMoves( const QRect& rect, GameRegistry* registry , QPainter *painter );

    /**
     * @brief A point value used to denote null. The value is not visible in the game's coordinate space
     */
    static const QPoint NullPoint;

    /**
     * @brief Sets a watermark whereby pieces whose pushedId is greater than the given value are painted using an
     * alternate color
     * @param pushIdDelineation If positive, push pieces whose pushedId is at or below this value are rendered in the
     * alternate color. A negative value disables color selection.
     */
    void setPushIdDelineation( int pushIdDelineation );

private:
    int mTileSize;
    int mPushIdDelineation;
};

#endif // BOARDRENDERER_H

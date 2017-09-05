#ifndef TILEDRAGMARKER_H
#define TILEDRAGMARKER_H

#include <QObject>
#include <QPoint>
#include <QRect>

QT_FORWARD_DECLARE_CLASS(QPainter)


class TileDragMarker : public QObject
{
    Q_OBJECT

public:
    TileDragMarker( QObject* parent = 0 );
    void render( const QRect* rect, QPainter* painter );

public slots:
    /**
     * @brief Show markers for the given angles at the screen coordinates
     * @param angleMask A bit mask of the four possible directional angles
     * @param center The screen coordinate to center the markers
     * @param tileSize The size of a tile (used as a size hint)
     */
    void enable( unsigned angleMask, QPoint center, int tileSize );

    /**
     * @brief Clear the markers from display
     */
    void disable();

signals:
    void rectDirty( QRect& rect );

private:
    QPoint mCenter;
    unsigned mAngleMask;
    int mTileSize;
    QRect mBounds;
};

#endif // TILEDRAGMARKER_H


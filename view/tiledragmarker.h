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
     * @param focusAngle An angle to select as the initial focus. Default is no selection
     */
    void enable( unsigned angleMask, QPoint center, int tileSize, int focusAngle = -1 );

    /**
     * @brief Clear the markers from display
     */
    void disable();

    /**
     * @brief Show which angle is currently selected
     * @param angle A 90 degree angle of exit. Focus will be applied if the given angle is enabled
     */
    void setFocus( int angle );

signals:
    void rectDirty( QRect& rect );

private:
    QPoint mCenter;
    unsigned mAngleMask;
    int mFocusAngle; // exit perspective value
    int mTileSize;
    QRect mBounds;
};

#endif // TILEDRAGMARKER_H


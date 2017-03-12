#ifndef SHOTVIEW_H
#define SHOTVIEW_H

#include <list>
#include <QObject>
#include <QRect>
#include <QColor>
#include <QPen>
#include <QPoint>
#include <QPainter>

class Shooter;

/**
 * @brief rendering logic for lazer shots
 */
class ShotView : public QObject
{
    Q_OBJECT

public:
    explicit ShotView(QObject *parent = 0);
    void reset();
    void render( QPainter* painter );

    /**
     * @brief setColor
     */
    void setColor( QColor color );

    /**
     * @brief Query whether this shot is rendering a splat yet
     * @return true if it is rendering a splat
     */
    bool hasTerminationPoint();

signals:
    void rectDirty( QRect& rect );

protected:
    /**
     * @brief trigger the shot
     * @param shooter Identifies the active point of origin
     */
    void commenceFire( Shooter* shooter );

    /**
     * @brief extend the shot to the given point
     * @param squareCenterPoint The point to extend to
     * @param startAngle The angle of entry into the new position
     * @param endAngle The angle of exit out of the new position
     */
    void grow( QPoint squareCenterPoint, int direction );

    /**
     * @brief Updates this shot so that it renders the splat
     * @param endAngle The final directional heading of the lazer beam
     * @param endOffset Offset (in pixels) to the point of termination
     */
    void addTermination(int endAngle, QPoint& hitPoint );

    QPoint getLeadPoint() const;
    Shooter* getShooter() const;

    bool shedTail();
    void killTheTank();

private:
    /**
     * @brief Instructs the shot to erase some of its tail. Up to a square length is erased or
     * up to the given point is erased. The target point must be a point along the trailing line segment.
     * @param target The point to erase toward.
     * @return true if the tail was reduced in size
     */
    bool trimToward(QPoint target);

    /**
     * @brief Notify that the rectangle comprised of the two points needs repainting.
     * The points can indicate any opposing corners of the rectangle.
     * @param p1 The first point
     * @param p2 The second point
     */
    void emitDirtySegment( QPoint p1, QPoint p2 );

    /**
     * @brief Notify that a rectangular area of the terminated end point (splat) needs repainting.
     */
    void emitSplatDirty();

    /**
     * @brief Get the trailing point (I.e. the tip of the tail)
     */
    QPoint getStartPoint();

    QPen mPen;
    std::list<QPoint> mBendPoints;
    Shooter* mShooter;
    QPoint mLeadPoint;
    QPoint mTailPoint;
    int mLeadAngle;
    int mTerminationAngle;
    bool mKillTheTank;
};

#endif // SHOTVIEW_H

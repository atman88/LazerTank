#ifndef SHOTVIEW_H
#define SHOTVIEW_H

#include <list>
#include <QObject>
#include <QRect>
#include <QColor>
#include <QPen>
#include <QPoint>
#include <QPainter>

#include "piecelistmanager.h"
#include "shooter.h"
#include "controller/animationaggregator.h"

class Game;

class ShotView : public QObject
{
    Q_OBJECT

public:
    explicit ShotView(QObject *parent = 0);
    PieceListManager* getPath();
    void setColor( QColor color );
    QColor getColor() const;
    void reset();
    void render( const QRect* rect, QPainter* painter );
    bool hasEndPoint();

signals:
    void rectDirty( QRect& rect );

protected:
    void commenceFire( Shooter* shooter );
    void grow( int col, int row, int startAngle, int endAngle );
    void growEnd( int endAngle, int endOffset );
    void shedTail();
    void showKill();
    bool commencing();

private:
    bool trimToward(QPoint target);
    void emitDirtySegment( QPoint p1, QPoint p2 );
    QPoint getStartPoint();

    QPen mPen;
    std::list<QPoint> mBendPoints;
    bool mKillShowing;
    QPoint mTailPoint;
    QPoint mEndPoint;
    int mEndAngle;
    Shooter* mShooter;
};

#endif // SHOTVIEW_H

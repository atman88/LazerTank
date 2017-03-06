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
#include "controller/animationstateaggregator.h"

class Game;

class ShotView : public QObject
{
    Q_OBJECT

public:
    explicit ShotView(QObject *parent = 0);
    PieceListManager* getPath();
    void setColor( QColor color );
    void reset();
    void render( QPainter* painter );
    bool hasTerminationPoint();

signals:
    void rectDirty( QRect& rect );

protected:
    void commenceFire( Shooter* shooter );
    void grow( int col, int row, int startAngle, int endAngle );
    void addTermination( int endAngle, int endOffset );
    bool shedTail();
    void killTheTank();

private:
    bool trimToward(QPoint target);
    void emitDirtySegment( QPoint p1, QPoint p2 );
    void emitSplatDirty();
    QPoint getStartPoint();

    QPen mPen;
    std::list<QPoint> mBendPoints;
    Shooter* mShooter;
    QPoint mLeadPoint;
    QPoint mTailPoint;
    int mTerminationAngle;
    bool mKillTheTank;
};

#endif // SHOTVIEW_H

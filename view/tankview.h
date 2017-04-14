#ifndef TANKVIEW_H
#define TANKVIEW_H

#include <QObject>
#include "shooter.h"
#include <controller/speedcontroller.h>

class TankView : public Shooter
{
    Q_OBJECT

public:
    explicit TankView(QObject *parent = 0);
    void init( GameRegistry* registry );
    void render( const QRect* rect, QPainter* painter );
    void pause();
    void resume();
    void stop();

signals:
    void changed( const QRect& rect );

public slots:
    void setViewX(const QVariant &x ) override;
    void setViewY(const QVariant &y ) override;
    void setViewRotation( const QVariant& angle ) override;

protected:
    void reset(const QPoint& p );
    virtual void onMoved( int col, int row, int angle ) = 0;

    RotateSpeedControlledAnimation mRotateAnimation;
    MoveSpeedControlledAnimation   mHorizontalAnimation;
    MoveSpeedControlledAnimation   mVerticalAnimation;

private:
    QRect mPreviousPaintRect;
};

#endif // TANKVIEW_H

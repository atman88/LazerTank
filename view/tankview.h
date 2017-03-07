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
    void init( Game* game );
    void render( const QRect* rect, QPainter* painter );
    void pause();
    void resume();
    void stop();

signals:
    void changed( const QRect& rect );

public slots:
    void setX(const QVariant &x ) override;
    void setY(const QVariant &y ) override;
    void setRotation( const QVariant& angle ) override;

protected:
    void reset( QPoint& p );
    virtual void onMoved( int col, int row ) = 0;

    RotateSpeedControlledAnimation mRotateAnimation;
    MoveSpeedControlledAnimation   mHorizontalAnimation;
    MoveSpeedControlledAnimation   mVerticalAnimation;

private:
    QRect mPreviousPaintRect;
};

#endif // TANKVIEW_H

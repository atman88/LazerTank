#ifndef TANKVIEW_H
#define TANKVIEW_H

#include <QObject>

#include "shooter.h"
#include "boardrenderer.h"
#include <controller/speedcontroller.h>

class TankView : public Shooter
{
    Q_OBJECT

public:
    explicit TankView( QObject *parent = nullptr );
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
    void onHighSpeedChanged( int speed );

protected:
    void reset(const ModelVector& v );
    virtual void onMoved( int col, int row, int angle ) = 0;

    RotateSpeedControlledAnimation mRotateAnimation;
    MoveSpeedControlledAnimation   mHorizontalAnimation;
    MoveSpeedControlledAnimation   mVerticalAnimation;

private:
    QRect mPreviousPaintRect;
    unsigned mPixmapType;
};

#endif // TANKVIEW_H

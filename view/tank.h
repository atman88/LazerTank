#ifndef TANK_H
#define TANK_H

#include <QObject>
#include <QPainter>
#include <QPropertyAnimation>

class Tank : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant rotation READ getRotation WRITE setRotation)
    Q_PROPERTY(QVariant x READ getX WRITE setX)
    Q_PROPERTY(QVariant y READ getY WRITE setY)

public:
    explicit Tank(QObject *parent = Q_NULLPTR );
    ~Tank() {
        if ( mRotateAnimation ) {
            delete mRotateAnimation;
        }
    }

    void paint( QPainter* painter );
    void move(int direction );
    bool isMoving();
    QVariant getRotation();
    QVariant getX();
    QVariant getY();
    QRect* getRect();

signals:
    void changed( QRect rect );
    void stopped();

public slots:
    void onUpdate( int x, int y );
    void setRotation(const QVariant &angle );
    void setX(const QVariant &x );
    void setY(const QVariant &y );
    void rotationAnimationFinished();
    void moveAnimationFinished();

private:
    QPixmap mPixmap;
    QRect mBoundingRect;
    QVariant mRotation;
    QPropertyAnimation* mRotateAnimation;
    QPropertyAnimation* mHorizontalAnimation;
    QPropertyAnimation* mVerticalAnimation;
};

#endif // TANK_H

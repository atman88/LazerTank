#ifndef SHOOTER_H
#define SHOOTER_H

#include <QObject>
#include <QVariant>
#include <QRect>

class Shooter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant rotation READ getRotation WRITE setRotation)
    Q_PROPERTY(QVariant x READ getX WRITE setX)
    Q_PROPERTY(QVariant y READ getY WRITE setY)

public:
    Shooter(QObject *parent = 0);
    virtual ~Shooter() {}

    void reset( QPoint& p );
    QVariant getRotation();
    QVariant getX();
    QVariant getY();
    const QRect& getRect();

public slots:
    virtual void setRotation(const QVariant &angle );
    virtual void setX(const QVariant &x );
    virtual void setY(const QVariant &y );

protected:
    QVariant mRotation;
    QRect mBoundingRect;
};

#endif // SHOOTER_H

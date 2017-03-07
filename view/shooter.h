#ifndef SHOOTER_H
#define SHOOTER_H

#include <QObject>
#include <QVariant>
#include <QRect>

class Game;
class BoardWindow;
class Shooter;

#include "model/shotmodel.h"

/**
 * @brief An abstract class defining the source of of a lazer beam
 */
class Shooter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant rotation READ getRotation WRITE setRotation)
    Q_PROPERTY(QVariant x READ getX WRITE setX)
    Q_PROPERTY(QVariant y READ getY WRITE setY)

public:
    Shooter(QObject *parent = 0);
    virtual ~Shooter() {}
    void init( Game* game, QColor color );

    void reset( QPoint p );

    /**
     * @brief fire this shooters shot
     */
    void fire();

    /**
     * @brief Release the trigger. Essentially a hint that its a good time to start shedding the shot's tail
     */
    void ceaseFire();

    ShotModel& getShot();
    QVariant getRotation();
    QVariant getX();
    QVariant getY();
    const QRect& getRect();

public slots:
    /**
     * @brief setRotation/setX/setY - These slots are solely for use by internal animations
     */
    virtual void setRotation(const QVariant &angle );
    virtual void setX(const QVariant &x );
    virtual void setY(const QVariant &y );

protected:
    QVariant mRotation;
    QRect mBoundingRect;

private:
    ShotModel mShot;
};

#endif // SHOOTER_H

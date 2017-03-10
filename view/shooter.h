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
 * @brief An abstract class defining the source view point of of a lazer beam
 */
class Shooter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant rotation READ getViewRotation WRITE setViewRotation)
    Q_PROPERTY(QVariant x READ getViewX WRITE setViewX)
    Q_PROPERTY(QVariant y READ getViewY WRITE setViewY)

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
    QVariant getViewRotation() const;
    QVariant getViewX() const;
    QVariant getViewY() const;
    const QRect& getRect();

public slots:
    /**
     * @brief setRotation/setX/setY - These slots are for use by internal animations
     */
    virtual void setViewRotation(const QVariant &angle );
    virtual void setViewX(const QVariant &x );
    virtual void setViewY(const QVariant &y );

protected:
    int mViewRotation;
    QRect mBoundingRect;

private:
    ShotModel mShot;
};

#endif // SHOOTER_H

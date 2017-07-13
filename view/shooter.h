#ifndef SHOOTER_H
#define SHOOTER_H

#include <QObject>
#include <QVariant>
#include <QRect>

class GameRegistry;
class BoardWindow;
class Shooter;

#include "model/piece.h"
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
    void init( GameRegistry* registry, PieceType type, QColor color );

    void reset( ModelVector v );

    /**
     * @brief fire this shooters shot
     * @return true if the shot was successfully started
     */
    bool fire();

    /**
     * @brief Release the trigger. Essentially a hint that its a good time to start shedding the shot's tail
     */
    void ceaseFire();

    /**
     * @brief Query the type of piece represented
     */
    PieceType getType() const;

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
    PieceType mType;
};

#endif // SHOOTER_H

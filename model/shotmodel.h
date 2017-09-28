#ifndef SHOTMODEL_H
#define SHOTMODEL_H

#include <QPropertyAnimation>

class Game;
class AnimationStateAggregator;
class Shooter;
class MeasureRunnable;

#include "model/modelpoint.h"
#include "view/shotview.h"

class ShotModel : public ShotView
{
    Q_OBJECT
    Q_PROPERTY(QVariant sequence READ getSequence WRITE setSequence)

public:
    explicit ShotModel(QObject *parent = 0);
    ~ShotModel();

    /**
     * @brief Initialization method
     * @param aggregate The aggregator this shot should participate in
     */
    void init( AnimationStateAggregator& aggregate );

    /**
     * @brief Fire the laser beam (i.e. shoot)
     * @param shooter Identifies the active point of origin
     * @return true if the shot was successfully started
     */
    bool fire( Shooter* shooter );

    /**
     * @brief Getter method needed for animation sequencing
     */
    QVariant getSequence();

    /**
     * @brief Return this shot to its inactive state
     */
    void reset();

    /**
     * @brief Get the number of squares the shot has travelled thus far
     * @return distance in terms of squares
     */
    int getDistance() const;

signals:
    /**
     * @brief Notifies that the laser beam hit the tank
     */
    void tankKilled();

public slots:
    /**
     * @brief Setter method used by the animation
     */
    void setSequence( const QVariant& shotSequence );

    /**
     * @brief Instructs that the beam should stop emitting. I.e. the trigger has been released.
     */
    void startShedding();

    /**
     * @brief Informs this shot that it has caused a kill
     */
    void setIsKill();

private:
    int getMeasurement() const;

    QVariant mSequence;
    QPropertyAnimation mAnimation;
    ModelVector mStartVector;
    ModelPoint mLeadingPoint;
    int mLeadingDirection;
    int mDistance;
    bool mShedding;
    int mKillSequence;
    MeasureRunnable* mRunnable;
    int mSpeed;

    friend class MeasureRunnable;
    friend class TestShotModel;
};

#endif // SHOTMODEL_H

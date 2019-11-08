#ifndef SHOTMODEL_H
#define SHOTMODEL_H

#include <QAbstractAnimation>

class Game;
class AnimationStateAggregator;
class Shooter;
class MeasureRunnable;

#include "model/modelpoint.h"
#include "view/shotview.h"

/**
 * @brief Animation for sequencing this shot
 */
class ShotAnimation : public QAbstractAnimation
{
    Q_OBJECT

public:
    int duration() const
    {
        return -1; // run until stopped
    }

signals:
    void currentTimeChanged( int currentTime );

protected:
    void updateCurrentTime( int currentTime )
    {
        emit currentTimeChanged( currentTime );
    }
};

class ShotModel : public ShotView
{
    Q_OBJECT

public:
    explicit ShotModel( QObject *parent = nullptr );
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
     * @brief Advance the animation sequence
     * @param sequence Animation iteration value
     */
    void onTimeChanged( int currentTime );

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

    ShotAnimation mAnimation;
    ModelVector mStartVector;
    ModelPoint mLeadingPoint;
    int mLeadingDirection;
    int mDistance;
    bool mShedding;
    int mKillSequence;
    int mLastStepNo;
    int mDuration;
    MeasureRunnable* mRunnable;

    friend class MeasureRunnable;
    friend class TestShotModel;
};

#endif // SHOTMODEL_H

#ifndef SHOTMODEL_H
#define SHOTMODEL_H

#include <QPropertyAnimation>

class Game;

#include "view/shotview.h"

class AnimationStateAggregator;
class Shooter;

class ShotModel : public ShotView
{
    Q_OBJECT
    Q_PROPERTY(QVariant sequence READ getSequence WRITE setSequence)

public:
    explicit ShotModel(QObject *parent = 0);

    /**
     * @brief Initialization method
     * @param aggregate The aggregator this shot should participate in
     */
    void init( AnimationStateAggregator* aggregate );

    /**
     * @brief Getter method needed for animation sequencing
     */
    QVariant getSequence();

    /**
     * @brief Return this shot to its inactive state
     */
    void reset();

    /**
     * @brief Get the column (x) where the laser beam has extended to
     * @return The column number or -1 if the shot is inactive
     */
    int getLeadingCol() const;
    void setLeadingCol(int leadingCol);

    /**
     * @brief Get the column (x) where the laser beam has extended to
     * @return The row number or -1 if the shot is inactive
     */
    int getLeadingRow() const;
    void setLeadingRow(int leadingRow);

signals:
    /**
     * @brief Notifies that the lazer beam hit the tank
     */
    void tankKilled();

public slots:
    /**
     * @brief Fire the laser beam (i.e. shoot)
     * @param shooter Identifies the active point of origin
     */
    void fire( Shooter* shooter );

    /**
     * @brief Setter method used by the animation
     */
    void setSequence( const QVariant& shotSequence );

    /**
     * @brief Instructs that the beam should stop emitting. I.e. the trigger has been released.
     */
    void startShedding();
    void setIsKill();

private:
    QVariant mSequence;
    QPropertyAnimation mAnimation;
    int mLeadingCol;
    int mLeadingRow;
    int mLeadingDirection;
    int mDistance;
    bool mShedding;
    int mKillSequence;
};

#endif // SHOTMODEL_H

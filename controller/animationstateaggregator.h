#ifndef ANIMATIONSTATEAGGREGATOR_H
#define ANIMATIONSTATEAGGREGATOR_H

#include <QObject>
#include <QAbstractAnimation>

/**
 * @brief The StateAggregator class
 * An aggregator combines multiple animation state changes into a a single super state.
 * If one or more connected objects are active, the aggregate state is Running.
 */
class AnimationStateAggregator : public QObject
{
    Q_OBJECT

public:
    explicit AnimationStateAggregator(QObject *parent = 0);

    /**
     * @brief active
     * @return true if one or more connected state objects are active
     */
    bool active();

signals:
    /**
     * @brief finished
     * Emmitted when the last connected state object transitions to finished
     */
    void finished();

public slots:
    /**
     * @brief onStateChanged
     * @param newState The new aggregate state
     * @param oldState The previous aggregate state
     */
    void onStateChanged(QAbstractAnimation::State newState, QAbstractAnimation::State oldState);

    /**
     * @brief Reset to default state without signaling
     */
    void reset();

private:
    int mActiveCount;
};

#endif // ANIMATIONSTATEAGGREGATOR_H

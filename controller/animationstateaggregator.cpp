#include <QVariant>
#include "animationstateaggregator.h"

AnimationStateAggregator::AnimationStateAggregator(QObject *parent) : QObject(parent), mActiveCount(0)
{
}

bool AnimationStateAggregator::active()
{
    return mActiveCount > 0;
}

void AnimationStateAggregator::onStateChanged( QAbstractAnimation::State newState, QAbstractAnimation::State oldState)
{
    if ( oldState == QAbstractAnimation::Stopped
      && newState == QAbstractAnimation::Running ) {
        ++mActiveCount;
    } else if ( oldState == QAbstractAnimation::Running
             && newState == QAbstractAnimation::Stopped ) {
        if ( --mActiveCount <= 0 ) {
            mActiveCount = 0;
            emit finished();
        }
    }
}

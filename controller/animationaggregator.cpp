#include "animationaggregator.h"

AnimationAggregator::AnimationAggregator(QObject *parent) : QObject(parent)
{
    mActiveCount = 0;
}

bool AnimationAggregator::active()
{
    return mActiveCount > 0;
}

void AnimationAggregator::onStateChanged(QAbstractAnimation::State newState, QAbstractAnimation::State oldState)
{
    if ( oldState == QAbstractAnimation::Stopped
      && newState == QAbstractAnimation::Running ) {
        ++mActiveCount;
    } else if ( oldState == QAbstractAnimation::Running
             && newState == QAbstractAnimation::Stopped ) {
        if ( --mActiveCount == 0 ) {
            emit finished();
        }
    }
}

#ifndef ANIMATIONAGGREGATOR_H
#define ANIMATIONAGGREGATOR_H

#include <QObject>
#include <QAbstractAnimation>

class AnimationAggregator : public QObject
{
    Q_OBJECT

public:
    explicit AnimationAggregator(QObject *parent = 0);

    bool active();

signals:
    void finished();

public slots:
    void onStateChanged(QAbstractAnimation::State newState, QAbstractAnimation::State oldState);

private:
    int mActiveCount;
};

#endif // ANIMATIONAGGREGATOR_H

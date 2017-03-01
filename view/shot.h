#ifndef SHOT_H
#define SHOT_H

#include <QObject>
#include <QRect>
#include <QPropertyAnimation>

#include "piecelistmanager.h"
#include "controller/animationaggregator.h"

class Game;

class Shot : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant sequence READ getSequence WRITE setSequence)

public:
    explicit Shot(QObject *parent = 0);
    void init(AnimationAggregator *aggregate);
    PieceListManager* getPath();
    QVariant getSequence();

signals:
    void tankKilled();


public slots:
    void setSequence( const QVariant& shotSequence );
    void fire( int direction );
    void stop();
    void reset();
    void setIsKill();

private:
    Game* getGame();
    QVariant mSequence;
    QPropertyAnimation* mAnimation;

    PieceListManager mPath;
    int mDirection;
    int mDistance;
    bool mStopping;
    bool mEndReached;
    int mKillSequence;
};

#endif // SHOT_H

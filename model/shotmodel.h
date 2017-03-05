#ifndef SHOTMODEL_H
#define SHOTMODEL_H

#include <QPropertyAnimation>

#include "view/shotview.h"
#include "view/shooter.h"

class ShotModel : public ShotView
{
    Q_OBJECT
    Q_PROPERTY(QVariant sequence READ getSequence WRITE setSequence)

public:
    explicit ShotModel(QObject *parent = 0);
    void init(AnimationStateAggregator *aggregate);
    QVariant getSequence();
    void reset();

    int getLeadingCol() const;
    void setLeadingCol(int leadingCol);

    int getLeadingRow() const;
    void setLeadingRow(int leadingRow);

    int getLeadingDirection() const;
    void setLeadingDirection(int leadingDirection);

signals:
    void tankKilled();

public slots:
    void fire( Shooter* shooter );
    void setSequence( const QVariant& shotSequence );
    void stop();
    void setIsKill();

private:
    Game* getGame();

    QVariant mSequence;
    QPropertyAnimation mAnimation;
    int mLeadingCol;
    int mLeadingRow;
    int mLeadingDirection;
    int mDistance;
    bool mStopping;
    int mKillSequence;
};

#endif // SHOTMODEL_H

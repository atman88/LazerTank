#ifndef SHOT_H
#define SHOT_H

#include <QObject>
#include <QRect>
#include <QPropertyAnimation>
#include "Game.h"
#include "piece.h"

class Shot : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant sequence READ getSequence WRITE setSequence)

public:
    explicit Shot(QObject *parent = 0);
    void init( Game* game );
    PieceList& getPath();
    QVariant getSequence();

signals:
    void stopped();
    void pathRemoved( Piece segment );
    void pathAdded( Piece segment );

public slots:
    void setSequence( const QVariant& shotSequence );
    void fire(int direction);
    void stop();

private:
    Game* getGame();
    QVariant mSequence;
    QPropertyAnimation* mAnimation;

    PieceList mPath;
    int mDirection;
    bool mStopping;
    bool mEndReached;
};

#endif // SHOT_H

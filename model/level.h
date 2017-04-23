#ifndef LEVEL_H
#define LEVEL_H

#include <QObject>
#include <QList>

#include "modelpoint.h"
#include "util/workerthread.h"

class GameRegistry;

class Level
{
public:
    Level( int getNumber );

    bool operator==( const Level& other ) const;
    bool operator<( const Level& other ) const;

    int getNumber() const;

private:
    int mNumber;
    ModelPoint mLowerRight;
};

class DirLoadRunnable;

class LevelList : public QObject
{
    Q_OBJECT

public:
    LevelList();

    void init( GameRegistry* registry );

    int nextLevel( int curLevel ) const;

    QList<Level> getList() const;

signals:
    void initialized();

private:
    DirLoadRunnable* mDirLoadRunnable;
    QList<Level> mList;

    friend class DirLoadRunnable;
};

#endif // LEVEL_H

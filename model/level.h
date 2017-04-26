#ifndef LEVEL_H
#define LEVEL_H

#include <QObject>
#include <QList>

#include "modelpoint.h"
#include "util/workerthread.h"

class GameRegistry;

/**
 * @brief Representation of a level and its attributes
 */
class Level
{
public:
    Level( int getNumber );

    bool operator==( const Level& other ) const;
    bool operator<( const Level& other ) const;

    /**
     * @brief Get the number for this level. The level number is both the displayed number and the key value.
     */
    int getNumber() const;

private:
    int mNumber;
    ModelPoint mLowerRight;
};

class DirLoadRunnable;

/**
 * @brief A level list container class
 */
class LevelList : public QObject
{
    Q_OBJECT

public:
    LevelList();

    void init( GameRegistry* registry );

    /**
     * @brief Get the number of the next available level
     * @param curLevel The previous level
     * @return The next level or 0 if no more levels available
     */
    int nextLevel( int curLevel ) const;

    QList<Level> getList() const;

signals:
    /**
     * @brief Notifies that initialization has completed
     */
    void initialized();

private:
    QList<Level> mList;

    friend class DirLoadRunnable;
};

#endif // LEVEL_H

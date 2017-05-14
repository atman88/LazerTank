#ifndef BOARDPOOL_H
#define BOARDPOOL_H

#include <map>
#include <QObject>

#include "model/board.h"

class LevelList;

class PoolLoadRunnable;

class BoardPool : public QObject
{
    Q_OBJECT

public:
    BoardPool( int visibleCount = 0, int size = 0 );
    ~BoardPool();
    void init( LevelList& levelList, int maxHeight );

    /**
     * @brief Retrieve the board for the given level, potentially trigging a background load.
     * @param level The level number identifying the board
     * @return The board if already present in the pool or 0 if not yet loaded.
     */
    Board* getBoard( int level );

    /**
     * @brief Get the board if it is already loaded
     * @param level The level number identifying the board
     * @return  The board if found otherwise 0.
     */
    Board* find( int level );

private slots:
    void onBoardLoaded( int number );

signals:
    /**
     * @brief Notifies that the board for the given level has been added to the pool
     * @param number The level number of the board
     */
    void boardLoaded( int number );

protected:
    std::map<int,Board*> mPool;
    int mFirstVisible;
    int mVisibleCount;

private:
    int ensureWithinVisible( int number );
    Board* getRecyclableBoard();

    unsigned mTotalSize;
    int mSize;
    PoolLoadRunnable* mRunnable;
};

#endif // BOARDPOOL_H

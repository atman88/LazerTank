#include <iostream>
#include "boardpool.h"
#include "controller/gameregistry.h"
#include "view/levelchooser.h"
#include "util/workerthread.h"

class PoolLoadRunnable : BasicRunnable
{
public:
    PoolLoadRunnable() : mBoard(nullptr), mLevel(0)
    {
    }

    void load( int level, Board* board );

    void run() override
    {
        mBoard->load( mLevel );
    }

    int getLevel()
    {
        return mLevel;
    }

    Board* reset()
    {
        Board* ret = mBoard;
        mBoard = nullptr;
        mLevel = 0;
        return ret;
    }

private:
    Board* mBoard;
    int mLevel;
};

BoardPool::BoardPool( int visibleCount, int size ) : QObject(nullptr), mFirstVisible(0), mVisibleCount(visibleCount),
  mTotalSize(visibleCount + (visibleCount>>1)), mSize(size), mRunnable(nullptr)
{
}

BoardPool::~BoardPool()
{
    delete mRunnable;
}

Board* BoardPool::find( int level )
{
    auto it = mPool.find( level );
    if ( it != mPool.end() ) {
        return it->second;
    }
    return nullptr;
}

void BoardPool::onBoardLoaded( int level )
{
    if ( mRunnable ) {
        if ( mRunnable->getLevel() == level ) {
            if ( Board* board = mRunnable->reset() ) {
                mPool[level] = board;
                emit boardLoaded( level );
            }
        }

        // load any next pending visible
        if ( !mRunnable->getLevel() ) {
            for( int level = mFirstVisible + mVisibleCount; --level >= mFirstVisible; ) {
                if ( !find( level ) ) {
                    mRunnable->load( level, getRecyclableBoard() );
                    break;
                }
            }
        }
    }
}

void BoardPool::init( LevelList& levelList, int maxHeight )
{
    if ( !mTotalSize ) {
        int maxLevel = levelList.size();
        int curHeight = 0;
        int curLevel;
        for( curLevel = 1; curHeight < maxHeight && curLevel < maxLevel; ++curLevel ) {
            if ( const Level* level = levelList.find( curLevel ) ) {
                curHeight += level->getSize().height();
            }
        }

        mVisibleCount = curLevel-1;
        mTotalSize = mVisibleCount + (mVisibleCount >> 1);
    }
}

int BoardPool::ensureWithinVisible( int level )
{
    // update visible tracking
    if ( !mFirstVisible ) {
        mFirstVisible = level;
    } else if ( level < mFirstVisible ) {
        mFirstVisible = level;
    } else if ( level >= mFirstVisible + mVisibleCount ) {
        mFirstVisible = level-mVisibleCount+1;
    }
    return mFirstVisible;
}

Board* BoardPool::getBoard( int level )
{
    ensureWithinVisible( level );

    if ( Board* board = find( level ) ) {
        return board;
    }

    if ( mRunnable ) {
        // check if currently loading a level
        if ( mRunnable->getLevel() ) {
            return nullptr;
        }
    } else {
        mRunnable = new PoolLoadRunnable();
    }

    mRunnable->load( level, getRecyclableBoard() );
    return nullptr;
}

Board* BoardPool::getRecyclableBoard()
{
    if ( mPool.size() < mTotalSize ) {
        auto board = new Board( this );
        QObject::connect( board, &Board::boardLoaded, this, &BoardPool::onBoardLoaded, Qt::QueuedConnection );
        return board;
    }
    int maxVisible = mFirstVisible + mVisibleCount - 1;

    int distance;
    int furthestLevel = 0;
    int furthestDistance = 0;
    for( auto it : mPool ) {
        int level = it.second->getLevel();
        if ( level < mFirstVisible ) {
            distance = mFirstVisible - level;
        } else if ( level > maxVisible ) {
            distance = level - maxVisible;
        } else {
            continue;
        }
        if ( !furthestDistance || distance > furthestDistance ) {
            furthestLevel = level;
            furthestDistance = distance;
        }
    }

    if ( furthestLevel ) {
        Board* board = mPool[furthestLevel];
        if ( mPool.erase( furthestLevel ) ) {
            return board;
        }
    }
    return nullptr;
}


void PoolLoadRunnable::load(int level, Board *board)
{
    if ( GameRegistry* registry = getRegistry(board) ) {
        Q_ASSERT( mBoard == nullptr );
        mBoard = board;
        mLevel = level;
        emit board->boardLoading( level );
        registry->getWorker().doWork( this );
    }
}

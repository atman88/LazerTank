#ifndef LEVEL_H
#define LEVEL_H

#include <QObject>
#include <QList>

#include "modelpoint.h"
#include "util/workerthread.h"

class GameRegistry;
class Board;

/**
 * @brief Representation of a level and its attributes
 */
class Level
{
public:
    explicit Level( int number, int width, int height );
    explicit Level() {}
    Level( const Level& other );
    ~Level() {}

    bool operator==( const Level& other ) const;
    bool operator<( const Level& other ) const;

    /**
     * @brief Get the number for this level. The level number is both the displayed number and the key value.
     */
    int getNumber() const;

    /**
     * @brief Get the size for this level in model space (i.e. number of columns and rows)
     */
    const QSize& getSize() const;

private:
    int mNumber;
    QSize mSize;
};

Q_DECLARE_METATYPE(Level)

/**
 * @brief Wrapper class for the list of levels
 */
class LevelList : public QObject
{
    Q_OBJECT

public:
    LevelList();
    void init( GameRegistry* registry );

    /**
     * @brief Obtain the list index for the given level number
     * @param number The level's number
     * @return the index if found, otherwise -1
     */
    int indexOf( int number ) const;

    /**
     * @brief look up a level by number
     * @param number The level's number
     * @return the level if found, otherwise 0
     */
    const Level* find( int number ) const;

    /**
     * @brief Retrieve the level for the given index
     * @param index offset within the list
     * @return the Level or 0 if the index is out of bounds
     */
    Level* at( int index ) const;

    /**
     * @brief Retrieve the level number at the given index
     * @param index offset within the list
     * @return the level number or 0 if the index is out of bounds
     */
    int numberAt( int index ) const;

    /**
     * @brief Define a level
     * @param number The number for the level
     * @param width the number of columns that this level's board occupies
     * @param height the number of rows that this level's board occupies
     */
    void addLevel( int number, int width, int height );

    /**
     * @brief Get the number of the next available level
     * @param curLevel The previous level. -1 finds the first (lowest) level.
     * @return The next level or 0 if no more levels available
     */
    int nextLevel( int curLevel ) const;

    /**
     * @brief The count of levels
     */
    int size() const;

    /**
     * @brief Query if initialized
     */
    bool isInitialized() const;

    QSize visualSizeHint() const;

private:
    bool mInitialized;
    QList<Level*> mLevels;
    QSize mVisualSizeHint;

signals:
    void initialized();

    friend class LevelXmlHandler;
    friend class ListLoadRunnable;
};

#endif // LEVEL_H

#ifndef LEVEL_H
#define LEVEL_H

#include <QObject>
#include <QList>
#include <QAbstractListModel>

#include "modelpoint.h"

class GameRegistry;
class Board;

/**
 * @brief Representation of a level and its attributes
 */
class Level
{
public:
    explicit Level( int number, int width, int height );
    explicit Level() : mNumber(0), mCompletedCount(0) {}
    Level( const Level& other ) = default;
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

    int getCompletedCount() const;
    void setCompletedCount( int completedCount );

private:
    int mNumber;
    QSize mSize;
    int mCompletedCount;
};

Q_DECLARE_METATYPE(Level)

/**
 * @brief Wrapper class for the list of levels
 */
class LevelList : public QAbstractListModel
{
    Q_OBJECT

public:
    LevelList();
    void init( GameRegistry* registry );

    int rowCount( const QModelIndex& ) const override;
    QVariant data( const QModelIndex& index, int role ) const override;

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
    const Level* at( int index ) const;

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
     * NOTE that levels should only be added in numerically increasing order.
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

    /**
     * @brief Query if the given level has been completed
     * @param number The level number
     * @return true if known to be completed otherwise false
     */
    bool isLevelCompleted( int number ) const;

    /**
     * @brief Get the total number of levels completed
     */
    int getCompletedCount() const;

    QSize visualSizeHint() const;

public slots:
    /**
     * @brief Mark that the user has completed this level
     * @param number The level to mark
     * @param moveCount The number of moves taken to complete the level
     */
    void setCompleted( int number, int moveCount );

signals:
    void initialized();
    void levelUpdated( const QModelIndex& index );

private:
    bool mInitialized;
    QList<Level> mLevels;
    QSize mVisualSizeHint;

    friend class LevelXmlHandler;
    friend class ListLoadRunnable;
};

#endif // LEVEL_H

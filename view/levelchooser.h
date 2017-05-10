#ifndef LEVELCHOOSER_H
#define LEVELCHOOSER_H

#include <QObject>
#include <QMenu>
#include <QWidget>
#include <QList>
#include "model/level.h"

class GameRegistry;

/**
 * @brief Wrapper class for the list of levels
 */
class LevelList : public QObject
{
    Q_OBJECT

public:
    LevelList();

    /**
     * @brief look up a level by number
     * @param number The level's number
     * @return the level if found, otherwise 0
     */
    Level* find( int number ) const;

    /**
     * @brief Retrieve the level # at the given index
     * @param index
     * @return the level number or 0 if the index is does not refer to a valid Level
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

    bool mInitialized;
    QList<Level*> mLevels;

signals:
    void initialized();

    friend class DirLoadRunnable;
};

class LevelChooser : public QMenu
{
    Q_OBJECT

public:
    explicit LevelChooser( QWidget* parent = 0 );
    void init( GameRegistry* registry );
    bool isListInitialized() const;
    bool isRealized() const;

    /**
     * @brief Prepares this menu for display use
     */
    void realize();

    /**
     * @brief look up a level by number
     * @param number The level's number
     * @return the level if found, otherwise 0
     */
    Level* find( int number ) const;

    /**
     * @brief Get the number of the next available level
     * @param curLevel The previous level. -1 finds the first (lowest) level.
     * @return The next level or 0 if no more levels available
     */
    int nextLevel( int curLevel ) const;

    /**
     * @brief Obtain the underlying list of levels
     */
    LevelList& getList();

signals:
    /**
     * @brief Notifies that initialization has completed
     */
    void listInitialized();

private slots:
    void onLevelListInitialized();
    void onBoardLoaded();

private:
    LevelList mLevelList;
    bool mRealized;
};

#endif // LEVELCHOOSER_H

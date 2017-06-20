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

    bool mInitialized;
    QList<Level*> mLevels;
    QSize mMinSize;
    QSize mMaxSize;

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
    bool isInitialized() const;
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
    const Level* find( int number ) const;

    /**
     * @brief Get the number of the next available level
     * @param curLevel The previous level. -1 finds the first (lowest) level.
     * @return The next level or 0 if no more levels available
     */
    int nextLevel( int curLevel ) const;

    /**
     * @brief Obtain the underlying list of levels
     */
    const LevelList& getList();

signals:
    /**
     * @brief Notifies that initialization has completed
     */
    void listInitialized();

protected:
    void setVisible( bool visible ) override;

    void setActiveIndex( int index );
    void keyPressEvent( QKeyEvent* event ) override;

private slots:
    void onLevelListInitialized();
    void onBoardLoaded( int number );

private:
    LevelList mLevelList;
    int mActiveIndex;
    bool mInitialized;
    bool mRealized;
};

#endif // LEVELCHOOSER_H

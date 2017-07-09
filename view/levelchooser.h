#ifndef LEVELCHOOSER_H
#define LEVELCHOOSER_H

#include <QObject>
#include <QListView>
#include "model/level.h"

class BoardPool;

class LevelChooser : public QListView
{
    Q_OBJECT

public:
    explicit LevelChooser( LevelList& levels, BoardPool& pool, QWidget* parent = 0 );

    /**
     * @brief Obtain the underlying list of levels
     */
    const LevelList* getList() const;

signals:
    /**
     * @brief Notifies that the user has selected a level
     */
    int levelChosen( int number );

private slots:
    void onActivated( const QModelIndex& index );

    void onBoardLoaded( int number );
};

#endif // LEVELCHOOSER_H

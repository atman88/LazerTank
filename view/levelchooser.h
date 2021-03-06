#ifndef LEVELCHOOSER_H
#define LEVELCHOOSER_H

#include <QObject>
#include <QListView>
#include "model/level.h"

constexpr int ChooserTileSize = 12;

class BoardPool;

class LevelChooser : public QListView
{
    Q_OBJECT

public:
    explicit LevelChooser( LevelList& levels, BoardPool& pool, QWidget* parent = nullptr );

    /**
     * @brief Query the display size of the list contents
     * @return
     */
    QSize preferredSize() const;

signals:
    /**
     * @brief Notifies that the user has selected a level
     */
    int levelChosen( int number );

public slots:
    /**
     * @brief Set selection to the given level
     * @param number
     */
    void setSelectedLevel( int number );

private slots:
    void onActivated( const QModelIndex& index );

    void onBoardLoaded( int number );

protected:
    void keyPressEvent(QKeyEvent* event) override;
};

#endif // LEVELCHOOSER_H

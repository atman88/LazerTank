#include <iostream>
#include <QSize>
#include <QScreen>

#include "levelchooser.h"
#include "boardwindow.h"
#include "controller/gameregistry.h"
#include "controller/game.h"
#include "model/level.h"
#include "model/board.h"
#include "model/boardpool.h"

#define TILE_SIZE 12

class LevelModel : public QAbstractListModel
{
public:
    LevelModel( LevelList& list ) : mList(list)
    {
    }

    int rowCount( const QModelIndex& ) const
    {
        return mList.size();
    }

    QVariant data( const QModelIndex& index, int role ) const
    {
        if ( index.row() >= 0 && index.row() < mList.size() && (role == Qt::DisplayRole || role == Qt::EditRole) ) {
            return mList.at( index.row() )->getNumber();
        }

        return QVariant();
    }

    LevelList& getList()
    {
        return mList;
    }

private:
    LevelList& mList;
};

LevelChooser::LevelChooser( LevelList& levels, QWidget* parent ) : QListView(parent)
{
    setWindowFlags( Qt::Dialog );
    setWindowTitle( "Select Level" );
    setWindowModality( Qt::ApplicationModal );

    LevelModel* model = new LevelModel( levels );
    model->setParent( this );
    setModel( model );

    QObject::connect( this, &LevelChooser::activated, this, &LevelChooser::onActivated );
}

void LevelChooser::onActivated( const QModelIndex& index )
{
    if ( int number = index.model()->data( index ).toInt() ) {
        emit levelChosen( number );
    }

    close();
}

const LevelList* LevelChooser::getList()
{
    if ( LevelModel* levelModel = dynamic_cast<LevelModel*>( model() ) ) {
        return &levelModel->getList();
    }
    return 0;
}

#ifndef PATHSEARCHACTION_H
#define PATHSEARCHACTION_H

#include <iostream>
#include <QAction>
#include <QObject>

#include "pathsearchcriteria.h"

/**
 * @brief A path search term container object
 */
class PathSearchAction : public QAction, public PathSearchCriteria
{
    Q_OBJECT
public:
    explicit PathSearchAction(QObject *parent = 0);

    /**
     * @brief Set criteria for this search
     * The the game's tank position is always used for the search starting coordinate
     * @param targetCol
     * @param targetRow
     * @param moveWhenFound
     */
    void setCriteria( PieceType focus, int targetCol, int targetRow, bool moveWhenFound );

    PathSearchCriteria* getCriteria();
};

#endif // PATHSEARCHACTION_H

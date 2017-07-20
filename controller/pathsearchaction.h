#ifndef PATHSEARCHACTION_H
#define PATHSEARCHACTION_H

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
    ~PathSearchAction()
    {
    }

    /**
     * @brief Set criteria for this search
     * The the game's tank position is always used for the search starting coordinate
     * @param focus The focus of the starting point. Either TANK or MOVE.
     * @param target The square to find a path to.
     * @return true if successful
     */
    bool setCriteria( PieceType focus, const ModelPoint& target );

    PathSearchCriteria* getCriteria();
};

#endif // PATHSEARCHACTION_H

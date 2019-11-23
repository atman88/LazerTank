#ifndef PATHSEARCHACTION_H
#define PATHSEARCHACTION_H

#include <QAction>
#include <QObject>

class GameRegistry;

#include "pathsearchcriteria.h"

/**
 * @brief A QAction which is enabled/disabled by a search term
 */
class PathSearchAction : public QAction
{
    Q_OBJECT
public:
    explicit PathSearchAction(QObject *parent = nullptr);
    ~PathSearchAction() override = default;

    /**
     * @brief Set criteria for this search
     * The starting point is determined by the focus parameter
     * @param focus Identifies the starting point. Either TANK or MOVE.
     * @param target The square to find a path to.
     * @return true if valid search criteria constructed successfully
     */
    bool setCriteria( PieceType focus, const ModelPoint& target );

    PathSearchCriteria& getCriteria();

private:
    PathSearchCriteria mCriteria;
};

#endif // PATHSEARCHACTION_H

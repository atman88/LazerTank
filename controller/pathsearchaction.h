#ifndef PATHSEARCHACTION_H
#define PATHSEARCHACTION_H

#include <iostream>
#include <QAction>
#include <QObject>

/**
 * @brief A path search term container object
 */
class PathSearchAction : public QAction
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
    void setCriteria( int targetCol, int targetRow, bool moveWhenFound );

    /**
     * @brief Getter methods
     */
    int getTargetCol() const;
    int getTargetRow() const;
    bool getMoveWhenFound() const;

private:
    int mTargetCol;
    int mTargetRow;
    bool mMoveWhenFound;
};

#endif // PATHSEARCHACTION_H

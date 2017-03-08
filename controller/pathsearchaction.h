#ifndef PATHSEARCHACTION_H
#define PATHSEARCHACTION_H

#include <iostream>
#include <QAction>
#include <QObject>

class PathSearchAction : public QAction
{
    Q_OBJECT
public:
    explicit PathSearchAction(QObject *parent = 0);

    void setCriteria( int getTargetCol, int getTargetRow, bool getMoveWhenFound );

    int getTargetCol() const;
    int getTargetRow() const;
    bool getMoveWhenFound() const;

private:
    int mTargetCol;
    int mTargetRow;
    bool mMoveWhenFound;
};

#endif // PATHSEARCHACTION_H

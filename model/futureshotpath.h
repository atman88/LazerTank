#ifndef FUTURESHOTPATH_H
#define FUTURESHOTPATH_H

#include <set>
#include <QObject>
#include <QRect>
#include <QPainterPath>

#include "piece.h"


class FutureShotPath : public QPainterPath
{
public:
    FutureShotPath( MovePiece* move );
    FutureShotPath( const FutureShotPath& source );

    int getUID() const;

    const QRect& getBounds();

private:
    const MovePiece* mMove;
    int mLeadingCol;
    int mLeadingRow;
    int mLeadingDirection;
    int mUID;
    QRect mBounds;

    friend struct FutureShotPathComparator;
    friend class FutureShotPathManager;
};

// comparator used by the stl
struct FutureShotPathComparator {
    bool operator() (const FutureShotPath& l, const FutureShotPath& r) const
    {
        return l.mUID < r.mUID;
    }
};

typedef std::set<FutureShotPath, FutureShotPathComparator> FutureShotPathSet;

class FutureShotPathManager : public QObject
{
    Q_OBJECT

public:
    FutureShotPathManager()
    {
    }

    void reset();

    const FutureShotPath* addPath( MovePiece* move );
    void removePath( Piece* piece );

    const FutureShotPathSet* getPaths() const;

signals:
    void dirtyRect( const QRect& rect );

private:
    FutureShotPathSet mPaths;
};

#endif // FUTURESHOTPATH_H

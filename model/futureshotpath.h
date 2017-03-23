#ifndef FUTURESHOTPATH_H
#define FUTURESHOTPATH_H

#include <set>
#include <QObject>
#include <QRect>
#include <QPainterPath>

#include "piece.h"

/**
 * @brief Models a pending shot sequence for a specific square and direction
 */
class FutureShotPath : public QPainterPath
{
public:
    /**
     * @brief Constructs a new instance and associates it with the given MovePiece via the unique identifier
     * @param move The MovePiece to associate
     * If the move is already associated with an existing instance, then a copy is constructed (which is
     * suitable for use as a search key).
     */
    FutureShotPath( MovePiece* move );

    /**
     * @brief Copy constructor
     * @param source The instance to copy
     */
    FutureShotPath( const FutureShotPath& source );

    /**
     * @brief Retrieves the unique identifier for this shot sequence
     * @return The unique non-zero identifier for the given instance
     */
    int getUID() const;

    /**
     * @brief Query the rectangular paint area that this object occupies
     */
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

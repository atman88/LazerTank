#ifndef FUTURESHOTPATH_H
#define FUTURESHOTPATH_H

#include <set>
#include <vector>
#include <QObject>
#include <QRect>

QT_FORWARD_DECLARE_CLASS(QPainterPath)

#include "modelpoint.h"
#include "piece.h"
#include "controller/futurechange.h"

/**
 * @brief A class that models a pending shot sequence for a specific square and direction
 */
class FutureShotPath
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

    ~FutureShotPath();

    /**
     * @brief Retrieves the unique identifier for this shot sequence
     * @return The unique non-zero identifier for the given instance
     */
    int getUID() const;

    /**
     * @brief toQPath
     * @return A QPainterPath depicting this shot
     */
    const QPainterPath* toQPath();

    /**
     * @brief Get the rectangle that indicates the paint region that this instance occupies
     */
    const QRect& getBounds() const;

    FutureShotPath& operator = ( const FutureShotPath& other );

private:
    /**
     * @brief initialize this instance's paint area rectangle
     */
    const QRect& initBounds();

    int mShotCount;
    ModelPoint mTailPoint;
    std::vector<ModelPoint> mBendPoints;
    ModelVector mLeadVector;
    int mUID;
    std::vector<FutureChange> mChanges;
    QRect mBounds;
    QPainterPath* mPainterPath;

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
    FutureShotPathManager() = default;

    void reset();

    const FutureShotPath* updateShots( int previousCount, MovePiece* move );
    void removePath( Piece* piece, bool undo );

    const FutureShotPathSet& getPaths() const;

    /**
     * @brief Cause a repaint of this path
     */
    void invalidate( const FutureShotPath& path );

signals:
    void dirtyRect( const QRect& rect );

private:
    FutureShotPathSet mPaths;
};

#endif // FUTURESHOTPATH_H

#ifndef PATHSEARCHCRITERIA_H
#define PATHSEARCHCRITERIA_H

#include "model/piece.h"

QT_FORWARD_DECLARE_CLASS(QAction)

class GameRegistry;
class TileDragTestResult;

class PathSearchCriteria
{
public:
    typedef enum {
        NullCriteria,
        PathCriteria,
        TileDragTestCriteria
    } CriteriaType;

    PathSearchCriteria();
    PathSearchCriteria( const PathSearchCriteria& source ) = default;
    ~PathSearchCriteria() = default;

    /**
     * @brief Initialize search criteria
     * The starting point is determined by the focus parameter
     * @param focus Identifies the starting point. Either TANK or MOVE.
     * @param target The square to find a path to
     * @param action A QAction to be controlled by the result of the testing this criteria
     * @return true if initialized successfully
     */
    bool setPathCriteria( PieceType focus, const ModelPoint& target, QAction* action );

    /**
     * @brief Set criteria for testing drag points for a target tile
     * The starting point is determined by the focus parameter
     * @param focus Identifies the starting point. Either TANK or MOVE.
     * @param target The tile piece to test
     * @param result Container which receives the results of the test. Note the game registry must be accessible via its hierarchy.
     * @return true if initialized successfully
     */
    bool setTileDragCriteria( PieceType focus, const Piece* target, TileDragTestResult* result );

    PathSearchCriteria& operator = ( const PathSearchCriteria other );
    bool operator == ( const PathSearchCriteria other );

    /**
     * @brief Getter methods
     */
    CriteriaType getCriteriaType() const;
    ModelPoint getStartPoint() const;
    ModelVector getStartVector() const;
    int getStartCol() const;
    int getStartRow() const;
    ModelPoint getTargetPoint() const;
    int getTargetCol() const;
    int getTargetRow() const;
    PieceType getFocus() const;
    TileDragTestResult* getTileDragTestResult() const;


    /**
     * @brief query whether this applies to the future
     * @return true if this applies to the future or false if it applies to the present
     */
    bool isFuturistic() const;

private:
    void setFocusInternal( PieceType focus, GameRegistry* registry );

protected:
    CriteriaType mCriteriaType;
    ModelVector mStartVector;
    ModelPoint mTargetPoint;
    PieceType mFocus;
    QAction* mAction;
    TileDragTestResult* mTileDragTestResult;
};

#endif // PATHSEARCHCRITERIA_H

#ifndef PATHSEARCHCRITERIA_H
#define PATHSEARCHCRITERIA_H

#include "model/piece.h"

class PathSearchCriteria
{
public:
    PathSearchCriteria();
    PathSearchCriteria( const PathSearchCriteria& source );
    ~PathSearchCriteria() {}

    PathSearchCriteria& operator = ( const PathSearchCriteria other );
    bool operator == ( const PathSearchCriteria other );

    /**
     * @brief Getter methods
     */
    int getStartCol() const;
    int getStartRow() const;
    int getStartDirection() const;
    int getTargetCol() const;
    int getTargetRow() const;
    PieceType getFocus() const;
    bool getMoveWhenFound() const;

    /**
     * @brief query whether this applies to the future
     * @return true if this applies to the future or false if it applies to the present
     */
    bool isFuturistic() const;

protected:
    int mStartCol;
    int mStartRow;
    int mStartDirection;
    int mTargetCol;
    int mTargetRow;
    PieceType mFocus;
    bool mMoveWhenFound;
};

#endif // PATHSEARCHCRITERIA_H
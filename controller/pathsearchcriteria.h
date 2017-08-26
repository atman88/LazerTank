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
    ModelVector getStartVector() const;
    int getStartCol() const;
    int getStartRow() const;
    int getStartDirection() const;
    ModelPoint getTargetPoint() const;
    int getTargetCol() const;
    int getTargetRow() const;
    PieceType getFocus() const;

    /**
     * @brief query whether this applies to the future
     * @return true if this applies to the future or false if it applies to the present
     */
    bool isFuturistic() const;

protected:
    ModelVector mStartVector;
    int mStartDirection;
    ModelPoint mTargetPoint;
    PieceType mFocus;
};

#endif // PATHSEARCHCRITERIA_H

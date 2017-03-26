#include "modelpoint.h"
#include "util/gameutils.h"

bool ModelPoint::isNull() const
{
    return mCol < 0 || mRow < 0;
}

bool ModelPoint::equals(ModelPoint &other)
{
    return mCol == other.mCol
        && mRow == other.mRow;
}

void ModelPoint::minMax(ModelPoint &min, ModelPoint &max) const
{
    if ( mCol < min.mCol ) min.mCol = mCol; else if ( mCol > max.mCol ) max.mCol = mCol;
    if ( mRow < min.mRow ) min.mRow = mRow; else if ( mRow > max.mRow ) max.mRow = mRow;
}

QPoint ModelPoint::toViewCenterSquare() const
{
    return modelToViewCenterSquare( mCol, mRow );
}

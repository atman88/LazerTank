#include "modelpoint.h"
#include "util/gameutils.h"

bool ModelPoint::isNull() const
{
    return mCol < 0 || mRow < 0;
}

void ModelPoint::setNull()
{
    mCol = mRow = -1;
}

bool ModelPoint::equals( const ModelPoint& other ) const
{
    return mCol == other.mCol && mRow == other.mRow;
}

bool ModelPoint::operator ==( const ModelPoint& other ) const
{
    return equals(other);
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

QPoint ModelPoint::toViewUpperLeft() const
{
    return QPoint( mCol*24, mRow* 24 );
}

bool ModelVector::equals(const ModelVector &other) const
{
    return mCol == other.mCol && mRow == other.mRow && mAngle == other.mAngle;
}

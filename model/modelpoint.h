#ifndef MODELPOINT_H
#define MODELPOINT_H

#include <QPoint>

class ModelPoint
{
public:
    /**
     * @brief Consruct a null model point
     */
    ModelPoint() : mCol(-1), mRow(-1)
    {
    }

    /**
     * @brief Construct a copy of a model point
     */
    ModelPoint( const ModelPoint& source ) : mCol(source.mCol), mRow(source.mRow)
    {
    }

    /**
     * @brief Construct a model point for the given square coordinates
     */
    ModelPoint( int col, int row ) : mCol(col), mRow(row)
    {
    }

    /**
     * @brief Query whether this point has been set to a model coordinate
     * @return true if not set to a model coordinate
     */
    bool isNull() const;

    /**
     * @brief compare equality
     */
    bool equals( ModelPoint& other );

    /**
     * @brief Update the given min and max points such that the bounds min,max contain this point.
     * Note this method assumes min.mCol <= max.mCol and min.mRow <= max.mRow.
     * @param min The miniumum point to update
     * @param max The maximum point to update
     */
    void minMax( ModelPoint &min, ModelPoint &max ) const;

    /**
     * @brief Obtain the view point corresponding to the center of of this square
     * @return
     */
    QPoint toViewCenterSquare() const;

    int mCol;
    int mRow;
};

#endif // MODELPOINT_H

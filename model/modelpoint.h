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
     * @brief Construct a ModelPoint from a view coordinate
     */
    explicit ModelPoint( const QPoint& source )  : mCol(source.x() / 24), mRow(source.y() / 24)
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
     * @brief Nullify this point
     */
    const ModelPoint& setNull();

    /**
     * @brief compare equality
     */
    bool equals( const ModelPoint& other ) const;
    bool operator == ( const ModelPoint& other ) const;
    bool operator != ( const ModelPoint& other ) const;

    /**
     * @brief Update the given min and max points such that the bounds min,max contain this point.
     * Note this method assumes min.mCol <= max.mCol and min.mRow <= max.mRow.
     * @param min The miniumum point to update
     * @param max The maximum point to update
     */
    void minMax( ModelPoint &min, ModelPoint &max ) const;

    /**
     * @brief Obtain the view point corresponding to the center of of this square
     */
    QPoint toViewCenterSquare() const;

    /**
     * @brief Generates a view point which corresponds to the upper left corner for this square
     */
    QPoint toViewUpperLeft() const;

    /**
     * @brief Generates a view point which corresponds to the lower right corner for this square
     */
    QPoint toViewLowerRight() const;

    int mCol;
    int mRow;
};

class ModelVector : public ModelPoint
{
public:
    ModelVector() : ModelPoint(-1,-1), mAngle(-1)
    {
    }

    ModelVector( int col, int row, int direction = 0 ) : ModelPoint(col,row), mAngle(direction)
    {
    }

    ModelVector( const ModelPoint& source, int direction = 0 ) : ModelPoint(source), mAngle(direction)
    {
    }

    ModelVector( const ModelVector& source ) : ModelPoint(source), mAngle(source.mAngle)
    {
    }

    void setPoint( ModelPoint p )
    {
        *dynamic_cast<ModelPoint*>(this) = p;
    }

    /**
     * @brief compare equality
     */
    bool equals( const ModelVector& other ) const;

    /**
     * @brief Obtain the view point corresponding to the point where a shot would enter this square for this vector
     */
    QPoint toViewEntryPoint() const;

    /**
     * @brief Obtain the view point corresponding to the point where a shot would exit this square for this vector
     */
    QPoint toViewExitPoint() const;

    int mAngle;
};

#endif // MODELPOINT_H

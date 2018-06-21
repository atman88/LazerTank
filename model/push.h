#ifndef PUSH_H
#define PUSH_H

#include "model/modelpoint.h"
#include "view/pushview.h"

class Push : public PushView
{
public:
    Push( QObject* parent = 0 );

    /**
     * @brief start a push sequence between two squares
     * @param what The type of piece to show being pushed
     * @param fromPoint The starting square
     * @param toPoint The destination square
     */
    void start( Piece& what, ModelPoint fromPoint, ModelPoint toPoint );

    ModelPoint getTargetPoint() const;

    /**
     * @brief tests whether the pushed piece covers the centerpoint of the given square
     * @param square The square to test
     * @return true if the square's center is covered
     */
    bool occupies( ModelPoint& square );

protected:
    void stopping() override;

private:
    ModelPoint mTargetPoint;
};

#endif // PUSH_H

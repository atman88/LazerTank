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

protected:
    void stopping() override;

private:
    ModelPoint mTargetPoint;
};

#endif // PUSH_H

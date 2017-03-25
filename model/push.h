#ifndef PUSH_H
#define PUSH_H

#include <view/pushview.h>

class Push : public PushView
{
public:
    Push( QObject* parent = 0 );

    /**
     * @brief start a push sequence between two squares
     * @param what The type of piece to show being pushed
     * @param fromCol starting column
     * @param fromRow starting row
     * @param toCol destination column
     * @param toRow destination row
     */
    void start( Piece& what, int fromCol, int fromRow, int toCol, int toRow );

    int getTargetCol() const;
    int getTargetRow() const;

protected:
    void stopping() override;

private:
    int mTargetCol;
    int mTargetRow;
};

#endif // PUSH_H

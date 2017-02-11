#ifndef INTENT_H
#define INTENT_H

#include "board.h"

typedef enum {
    MOVE
} IntentType;

class Intent
{
public:
    Intent( IntentType type, int x, int y, int angle = 0 ) : mType(type), mX(x), mY(y), mAngle(angle)
    {
    }
    Intent( const Intent& from )
    {
        mType = from.mType;
        mAngle = from.mAngle;
        mX = from.mX;
        mY = from.mY;
    }

    friend bool operator<(const Intent& l, const Intent& r)
    {
        return l.encodedPos() < r.encodedPos();
    }
    IntentType getType() const;
    int getX() const;
    int getY() const;
    int getAngle() const;

    static int encodePos( int x, int y )
    {
        return y * BOARD_MAX_WIDTH + x;
    }

    int encodedPos() const
    {
        return encodePos( mX, mY );
    }

private:
    IntentType mType;
    int mX;
    int mY;
    int mAngle;
};

#endif // INTENT_H

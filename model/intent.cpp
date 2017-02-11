#include "intent.h"

IntentType Intent::getType() const
{
    return mType;
}

int Intent::getX() const
{
    return mX;
}

int Intent::getY() const
{
    return mY;
}

int Intent::getAngle() const
{
    return mAngle;
}

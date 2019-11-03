#include <QRect>

#include "piece.h"

MovePiece::MovePiece( const Piece* source ) : SimplePiece(source)
{
    mShotCount = source->getShotCount();
    mPreviousPushedId = source->getPreviousPushedId();

    if ( source->hasPush() || mShotCount )
        if ( const MovePiece* pusherSource = dynamic_cast<const MovePiece*>(source) ) {
            mPushPieceType    = pusherSource->mPushPieceType;
            mPushPieceAngle   = pusherSource->mPushPieceAngle;
            mShotPathUID      = pusherSource->mShotPathUID;
            return;
        }

    mPushPieceType  = NONE;
    mPushPieceAngle = 0;
    mShotPathUID = 0;
}

bool MovePiece::hasPush() const
{
    return mPushPieceType != NONE;
}

PieceType MovePiece::getPushPieceType() const
{
    return mPushPieceType;
}

int MovePiece::getPushPieceAngle() const
{
    return mPushPieceAngle;
}

int MovePiece::getShotCount() const
{
    return mShotCount;
}

int MovePiece::getPreviousPushedId() const
{
    return mPreviousPushedId;
}

bool MovePiece::setShotCount( int count )
{
    if ( count != mShotCount ) {
        mShotCount = count;
        return true;
    }
    return false;
}

int MovePiece::decrementShots()
{
    if ( --mShotCount < 0 ) {
        mShotCount = 0;
        return -1;
    }
    return mShotCount;
}

int MovePiece::getShotPathUID() const
{
    return mShotPathUID;
}

void MovePiece::setShotPathUID(int shotPathUID)
{
    mShotPathUID = shotPathUID;
}

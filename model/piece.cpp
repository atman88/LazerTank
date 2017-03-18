#include <QRect>

#include "piece.h"

PusherPiece::PusherPiece( const Piece* source ) : SimplePiece(source)
{
    if ( source->hasPush() ) {
        const PusherPiece* pusherSource = dynamic_cast<const PusherPiece*>(source);
        mPushPieceType  = pusherSource->mPushPieceType;
        mPushPieceAngle = pusherSource->mPushPieceAngle;
    } else {
        mPushPieceType  = NONE;
        mPushPieceAngle = 0;
    }
}

PieceType PusherPiece::getPushPieceType() const
{
    return mPushPieceType;
}

int PusherPiece::getPushPieceAngle() const
{
    return mPushPieceAngle;
}

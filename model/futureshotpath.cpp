#include "futureshotpath.h"
#include "controller/game.h"
#include "util/gameutils.h"

FutureShotPath::FutureShotPath( MovePiece* move ) : mMove(move), mTailPoint(move->getCol(),move->getRow()),
  mLeadPoint(move->getCol(),move->getRow()), mLeadingDirection(move->getAngle()), mUID(move->getShotPathUID()),
  mPainterPath(0)
{
    if ( !mUID ) {
        static int lastUID = 0;
        move->setShotPathUID(++lastUID);
        mUID = lastUID;
    }
}

FutureShotPath::FutureShotPath( const FutureShotPath& source ) : mMove(source.mMove), mTailPoint(source.mTailPoint),
  mBendPoints(source.mBendPoints), mLeadPoint(source.mLeadPoint), mLeadingDirection(source.mLeadingDirection),
  mUID(source.mUID), mBounds(source.mBounds), mPainterPath(0)
{
}

FutureShotPath::~FutureShotPath()
{
    if ( mPainterPath ) {
        delete mPainterPath;
    }
}

int FutureShotPath::getUID() const
{
    return mUID;
}

const QRect& FutureShotPath::getBounds()
{
    if ( mBounds.isNull() ) {
        ModelPoint min = mTailPoint;
        ModelPoint max = mTailPoint;
        for( auto it : mBendPoints ) {
            it.minMax( min, max );
        }
        mLeadPoint.minMax( min, max );
        mBounds.setTopLeft( min.toViewCenterSquare() );
        mBounds.setBottomRight( max.toViewCenterSquare() );
        mBounds += QMargins(1,1,1,1);
    }
    return mBounds;
}

const QPainterPath* FutureShotPath::toQPath()
{
    if ( !mPainterPath ) {
        mPainterPath = new QPainterPath();
        QPoint leadingPoint;
        mPainterPath->moveTo( mTailPoint.toViewCenterSquare() );
        for( auto it : mBendPoints ) {
            mPainterPath->lineTo( it.toViewCenterSquare() );
        }
        mPainterPath->moveTo( mLeadPoint.toViewCenterSquare() );
    }
    return mPainterPath;
}

void FutureShotPathManager::reset()
{
    mPaths.clear();
}

const FutureShotPath* FutureShotPathManager::addPath( MovePiece* move )
{
    if ( Game* game = getGame(this) ) {
        FutureShotPath path(move);
        FutureChange *previousChange = 0;

        ModelPoint leadPoint( path.mLeadPoint );
        int leadingDirection = path.mLeadingDirection;

        int shotsRemaining = move->getShotCount();
        while( shotsRemaining > 0 ) {
            path.mLeadPoint = leadPoint;
            if ( !getAdjacentPosition( leadingDirection, &leadPoint.mCol, &leadPoint.mRow ) ) {
                break;
            }

            FutureChange curChange;
            curChange.changeType = NO_CHANGE;
            if ( !game->canShootThru( leadPoint.mCol, leadPoint.mRow, &leadingDirection, &curChange ) ) {
                if ( curChange.changeType == NO_CHANGE ) {
                    break;
                }
                if ( previousChange
                  && previousChange->changeType == PIECE_PUSHED
                  && curChange.changeType == PIECE_PUSHED
                  && previousChange->endCoord.equals( path.mLeadPoint ) ) {
                    ++previousChange->u.multiPush.count;
                } else {
                    std::vector<FutureChange>::iterator ret = path.mChanges.insert( path.mChanges.end(), curChange );
                    previousChange = &(*ret);

                    // need to resume without advancing the lead point in the case of a tile decay:
                    if ( curChange.changeType == TILE_CHANGE && curChange.u.tileType == WOOD ) {
                        leadPoint = path.mLeadPoint;
                    }
                }
                --shotsRemaining;
            }

            if ( leadingDirection != path.mLeadingDirection ) {
                path.mLeadingDirection = leadingDirection;
                path.mBendPoints.push_back( leadPoint );
            }
        }
        path.mLeadPoint = leadPoint;

        std::pair<std::set<FutureShotPath>::iterator,bool> ret = mPaths.insert( path );
        if ( ret.second ) {
            emit dirtyRect( path.getBounds() );
            return &(*ret.first);
        }
    }
    return 0;
}

void FutureShotPathManager::removePath( Piece* piece )
{
    if ( MovePiece* move = dynamic_cast<MovePiece*>(piece) ) {
        FutureShotPath key( move );
        auto it = mPaths.find( key );
        if ( it != mPaths.end() ) {
            emit dirtyRect( it->mBounds );
            mPaths.erase( it );
        }
    }
}

const FutureShotPathSet* FutureShotPathManager::getPaths() const
{
    return &mPaths;
}

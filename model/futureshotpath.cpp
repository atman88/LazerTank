#include "futureshotpath.h"
#include "controller/game.h"
#include "util/gameutils.h"

FutureShotPath::FutureShotPath( MovePiece* move ) : mMove(move), mShotCount(0), mTailPoint(move->getCol(),move->getRow()),
  mLeadPoint(move->getCol(),move->getRow()), mLeadingDirection(move->getAngle()), mUID(move->getShotPathUID()),
  mPainterPath(0)
{
    if ( !mUID ) {
        static int lastUID = 0;
        move->setShotPathUID(++lastUID);
        mUID = lastUID;
    }
}

FutureShotPath::FutureShotPath( const FutureShotPath& source ) : mMove(source.mMove), mShotCount(source.mShotCount),
  mTailPoint(source.mTailPoint), mBendPoints(source.mBendPoints), mLeadPoint(source.mLeadPoint),
  mLeadingDirection(source.mLeadingDirection), mUID(source.mUID), mChanges(source.mChanges), mBounds(source.mBounds),
  mPainterPath(0)
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

const QRect& FutureShotPath::initBounds()
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

const QRect& FutureShotPath::getBounds() const
{
    return mBounds;
}

FutureShotPath &FutureShotPath::operator =( const FutureShotPath &other )
{
    mMove             = other.mMove;
    mShotCount        = other.mShotCount;
    mTailPoint        = other.mTailPoint;
    mBendPoints       = other.mBendPoints;
    mLeadPoint        = other.mLeadPoint;
    mLeadingDirection = other.mLeadingDirection;
    mUID              = other.mUID;
    mBounds           = other.mBounds;
    mPainterPath      = 0;

    return *this;
}

const QPainterPath* FutureShotPath::toQPath()
{
    if ( !mPainterPath ) {
        mPainterPath = new QPainterPath();
        mPainterPath->moveTo( mTailPoint.toViewCenterSquare() );
        for( auto it : mBendPoints ) {
            mPainterPath->lineTo( it.toViewCenterSquare() );
        }
        mPainterPath->lineTo( mLeadPoint.toViewCenterSquare() );
    }
    return mPainterPath;
}

void FutureShotPathManager::reset()
{
    mPaths.clear();
}

const FutureShotPath* FutureShotPathManager::updatePath( MovePiece* move )
{
    if ( Game* game = getGame(this) ) {
        int shotCount = move->getShotCount();

        FutureShotPath path(move);
        FutureShotPathSet::iterator it = mPaths.find( path );
        if ( it != mPaths.end() ) {
            if ( it->mShotCount <= shotCount ) {
                path = *it;
            } else {
                game->getBoard(true)->undoChanges( it->mChanges );
                emit dirtyRect( it->getBounds() );
            }
            mPaths.erase( it );
        }

        if ( !shotCount ) {
            move->setShotPathUID(0);
            return 0;
        }

        bool havePreviousChange = false;
        std::vector<FutureChange>::iterator previousChange;

        ModelPoint leadPoint( path.mLeadPoint );
        int leadingDirection = path.mLeadingDirection;

        while( path.mShotCount < shotCount ) {
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
                if ( havePreviousChange
                  && previousChange->changeType == PIECE_PUSHED
                  && curChange.changeType == PIECE_PUSHED
                  && previousChange->point.equals( leadPoint ) ) {
                    curChange.u.multiPush.count += previousChange->u.multiPush.count;
                    path.mChanges.erase( previousChange );
                }

                std::vector<FutureChange>::iterator ret = path.mChanges.insert( path.mChanges.end(), curChange );
                havePreviousChange = true;
                previousChange = ret;

                // need to resume without advancing the lead point in the case of a tile decay:
                if ( curChange.changeType == TILE_CHANGE && curChange.u.tileType == WOOD ) {
                    leadPoint = path.mLeadPoint;
                }
                ++path.mShotCount;
            }

            if ( leadingDirection != path.mLeadingDirection ) {
                path.mLeadingDirection = leadingDirection;
                path.mBendPoints.push_back( leadPoint );
            }
        }
        path.mLeadPoint = leadPoint;
        path.mShotCount = shotCount;

        emit dirtyRect( path.initBounds() );

        std::pair<std::set<FutureShotPath>::iterator,bool> ret = mPaths.insert( path );
        if ( ret.second ) {
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

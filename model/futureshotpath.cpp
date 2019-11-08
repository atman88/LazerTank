#include <iostream>
#include <QPainterPath>

#include "futureshotpath.h"
#include "controller/game.h"
#include "util/gameutils.h"

FutureShotPath::FutureShotPath( MovePiece* move ) : mShotCount(0), mTailPoint(*move), mLeadVector(*move),
  mUID(move->getShotPathUID()), mPainterPath(nullptr)
{
    if ( !mUID ) {
        static int lastUID = 0;
        move->setShotPathUID(++lastUID);
        mUID = lastUID;
    }
}

FutureShotPath::FutureShotPath( const FutureShotPath& source ) : mShotCount(source.mShotCount),
  mTailPoint(source.mTailPoint), mBendPoints(source.mBendPoints), mLeadVector(source.mLeadVector),
  mUID(source.mUID), mChanges(source.mChanges), mBounds(source.mBounds), mPainterPath(nullptr)
{
}

FutureShotPath::~FutureShotPath()
{
    delete mPainterPath;
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
        for( const auto& it : mBendPoints ) {
            it.minMax( min, max );
        }
        mLeadVector.minMax( min, max );
        mBounds.setTopLeft( min.toViewUpperLeft() );
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
    mShotCount        = other.mShotCount;
    mTailPoint        = other.mTailPoint;
    mBendPoints       = other.mBendPoints;
    mLeadVector        = other.mLeadVector;
    mUID              = other.mUID;
    mChanges          = other.mChanges;
    mBounds           = other.mBounds;
    mPainterPath      = nullptr;

    return *this;
}

const QPainterPath* FutureShotPath::toQPath()
{
    if ( !mPainterPath ) {
        mPainterPath = new QPainterPath();
        mPainterPath->moveTo( mTailPoint.toViewCenterSquare() );
        for( const auto& it : mBendPoints ) {
            mPainterPath->lineTo( it.toViewCenterSquare() );
        }
        bool lastShotHasPush = false;
        if ( !mChanges.empty() ) {
            FutureChange lastChange = mChanges.back();\
            if ( lastChange.changeType == PIECE_PUSHED && lastChange.point.equals( mLeadVector ) ) {
                int totalPushCount = 0;
                for( auto it : mChanges ) {
                    if ( it.changeType == PIECE_PUSHED ) {
                        totalPushCount += it.u.multiPush.count;
                    }
                }
                lastShotHasPush = (totalPushCount == mShotCount);
            }
        }
        mPainterPath->lineTo( lastShotHasPush ? mLeadVector.toViewExitPoint() : mLeadVector.toViewEntryPoint() );
    }
    return mPainterPath;
}

void FutureShotPathManager::reset()
{
    mPaths.clear();
}

void FutureShotPathManager::invalidate( const FutureShotPath& path )
{
    emit dirtyRect( path.getBounds() );
}

const FutureShotPath* FutureShotPathManager::updateShots( int previousCount, MovePiece* move )
{
    if ( GameRegistry* registry = getRegistry(this) ) {
        FutureShotPath path(move);
        auto it = mPaths.find( path );
        Board* board = registry->getGame().getBoard(true);
        if ( it != mPaths.end() ) {
            board->undoChanges( previousCount, it->mChanges );
            emit dirtyRect( it->getBounds() );
            mPaths.erase( it );
        }


        int newCount = move->getShotCount();
        if ( !newCount ) {
            move->setShotPathUID(0);
            return nullptr;
        }

        bool havePreviousChange = false;
        std::vector<FutureChange>::iterator previousChange;

        ModelVector leadVector( path.mLeadVector );
        unsigned maxBends = board->getWidth() + board->getHeight();

        while( path.mShotCount < newCount ) {
            path.mLeadVector = leadVector;
            if ( !getAdjacentPosition( leadVector.mAngle, &leadVector ) ) {
                break;
            }

            FutureChange curChange;
            curChange.changeType = NO_CHANGE;
            if ( !registry->getGame().canShootThru( leadVector, &leadVector.mAngle, &curChange, true ) ) {
                if ( curChange.changeType == NO_CHANGE ) {
                    break;
                }
                if ( havePreviousChange
                  && previousChange->changeType == PIECE_PUSHED
                  && curChange.changeType == PIECE_PUSHED
                  && previousChange->point.equals( leadVector ) ) {
                    curChange.u.multiPush.count += previousChange->u.multiPush.count;
                    curChange.u.multiPush.previousPushedId = previousChange->u.multiPush.previousPushedId;
                    path.mChanges.erase( previousChange );
                }

                auto ret = path.mChanges.insert( path.mChanges.end(), curChange );
                havePreviousChange = true;
                previousChange = ret;

                ++path.mShotCount;
                if ( curChange.changeType == TILE_CHANGE && (curChange.u.tileType == WOOD || curChange.u.tileType == WOOD_DAMAGED) ) {
                    // resume without advancing the lead point given the current point is still obstructed:
                    leadVector = path.mLeadVector;
                } else if ( !path.mBendPoints.empty() && path.mShotCount < newCount ) {
                    // resume from beginning to handle case where the previous change affected our path (cannot optimize)
                    leadVector = *move;
                    path.mBendPoints.clear();
                }
            } else if ( leadVector.mAngle != path.mLeadVector.mAngle ) {
                path.mLeadVector.mAngle = leadVector.mAngle;
                if ( path.mBendPoints.size() >= maxBends ) { // safety
                    std::cout << "** max bends " << maxBends << " exceeded" << std::endl;
                    break;
                }
                path.mBendPoints.push_back( leadVector );
            }
        }
        path.mLeadVector = leadVector;
        path.mShotCount = newCount;

        path.initBounds();
        invalidate( path );

        std::pair<std::set<FutureShotPath>::iterator,bool> ret = mPaths.insert( path );
        if ( ret.second ) {
            return &(*ret.first);
        }
    }
    return nullptr;
}

void FutureShotPathManager::removePath( Piece* piece, bool undo )
{
    if ( auto move = dynamic_cast<MovePiece*>(piece) ) {
        FutureShotPath key( move );
        auto it = mPaths.find( key );
        if ( it != mPaths.end() ) {
            QRect bounds( it->mBounds );
            if ( undo ) {
                if ( GameRegistry* registry = getRegistry(this) ) {
                    registry->getGame().getBoard(true)->undoChanges( move->getShotCount(), it->mChanges );
                }
            }
            mPaths.erase( it );
            emit dirtyRect( bounds );
        }
    }
}

const FutureShotPathSet& FutureShotPathManager::getPaths() const
{
    return mPaths;
}

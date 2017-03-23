#include "futureshotpath.h"
#include "controller/game.h"
#include "util/gameutils.h"

FutureShotPath::FutureShotPath( MovePiece* move ) : mMove(move), mLeadingCol(move->getCol()),
  mLeadingRow(move->getRow()), mLeadingDirection(move->getAngle())
{
    if ( !(mUID = move->getShotPathUID()) ) {
        static int lastUID = 0;
        move->setShotPathUID(++lastUID);
        mUID = lastUID;
    }
}

FutureShotPath::FutureShotPath( const FutureShotPath& source ) : QPainterPath(source), mMove(source.mMove),
  mLeadingCol(source.mLeadingCol), mLeadingRow(source.mLeadingRow), mLeadingDirection(source.mLeadingDirection),
  mUID(source.mUID)
{
}

int FutureShotPath::getUID() const
{
    return mUID;
}

const QRect& FutureShotPath::getBounds()
{
    if ( mBounds.isNull() ) {
        mBounds = boundingRect().toRect() + QMargins(1,1,1,1);
    }
    return mBounds;
}

void FutureShotPathManager::reset()
{
    mPaths.clear();
}

const FutureShotPath* FutureShotPathManager::addPath( MovePiece* move )
{
    if ( Game* game = getGame(this) ) {
        FutureShotPath path(move);
        move->setShotPathUID( path.getUID() );

        int leadingCol = path.mLeadingCol;
        int leadingRow = path.mLeadingRow;
        int leadingDirection = path.mLeadingDirection;
        QPoint leadingPoint( modelToViewCenterSquare(leadingCol,leadingRow) );
        path.moveTo( leadingPoint );
//        QRect bounds(leadingPoint,QSize(1,1));

        while( getAdjacentPosition( leadingDirection, &leadingCol, &leadingRow ) ) {
            leadingPoint = modelToViewCenterSquare(leadingCol,leadingRow);
            if ( !game->canShootThru( leadingCol, leadingRow, &leadingDirection, 0, &leadingPoint ) ) {
                break;
            }
            if ( leadingDirection != path.mLeadingDirection ) {
                path.mLeadingDirection = leadingDirection;
                path.lineTo( leadingPoint );
//                bounds |= QRect( leadingPoint, QSize(1,1) );
            }
        }
        path.mLeadingCol = leadingCol;
        path.mLeadingRow = leadingRow;
        path.lineTo( leadingPoint );
//        bounds |= QRect( leadingPoint, QSize(1,1) );

        auto ret = mPaths.insert( path );
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

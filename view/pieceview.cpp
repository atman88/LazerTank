#include "pieceview.h"
#include "util/renderutils.h"

PieceView::PieceView( const PieceView *source )
{
    mType      = source->mType;
    mCol       = source->mCol;
    mRow       = source->mRow;
    mAngle     = source->mAngle;
    mShotCount = source->mShotCount;
}

PieceType PieceView::getType() const
{
    return mType;
}

int PieceView::getCol() const
{
    return mCol;
}

int PieceView::getRow() const
{
    return mRow;
}

int PieceView::getAngle() const
{
    return mAngle;
}

void PieceView::setType( PieceType type )
{
    mType = type;
}

void PieceView::setAngle( int angle )
{
    mAngle = angle;
}

int PieceView::incrementShots()
{
    return ++mShotCount;
}

bool PieceView::decrementShots()
{
    if ( --mShotCount < 0 ) {
        mShotCount = 0;
        return false;
    }
    return true;
}

void PieceView::getBounds( QRect *rect ) const
{
    rect->setRect( mCol*24, mRow*24, 24, 24 );
}

bool PieceView::render( const QRect *dirty, QPainter *painter )
{
    QRect bounds;
    getBounds( &bounds );
    if ( dirty->intersects( bounds ) ) {
        renderPiece( mType, bounds.left(), bounds.top(), mAngle, painter );
        if ( mShotCount ) {
            painter->drawText( bounds, Qt::AlignCenter, QString::number(mShotCount) );
        }
        return true;
    }
    return bounds.top() <= dirty->bottom();
}

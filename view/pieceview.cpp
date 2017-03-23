#include "pieceview.h"
#include "util/renderutils.h"

PieceView::PieceView( const PieceView *source )
{
    mType      = source->mType;
    mCol       = source->mCol;
    mRow       = source->mRow;
    mAngle     = source->mAngle;
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
        int shotCount = getShotCount();
        if ( shotCount > 1 ) {
            QPen pen;
            pen.setColor( QColor(Qt::darkBlue) );
            painter->setPen( pen );
            painter->drawText( bounds, Qt::AlignCenter, QString::number(shotCount) );
        }
        return true;
    }
    return bounds.top() <= dirty->bottom();
}

#include "pieceview.h"
#include "boardrenderer.h"

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

bool PieceView::render( const QRect *dirty, const BoardRenderer& renderer, QPainter *painter )
{
    QRect bounds;
    renderer.getBounds( *this, &bounds );
    if ( dirty->intersects( bounds ) ) {
        renderer.renderPiece( mType, bounds, mAngle, painter );
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

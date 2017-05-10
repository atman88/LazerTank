#include <iostream>
#include "boardrenderer.h"
#include "model/board.h"
#include "util/imageutils.h"

const QPoint BoardRenderer::NullPoint = QPoint(-1,-1);

BoardRenderer::BoardRenderer( int tileSize ) : mTileSize(tileSize)
{
}

int BoardRenderer::tileSize() const
{
    return mTileSize;
}

void BoardRenderer::render( const QRect* rect, Board* board, QPainter* painter ) const
{
    int minCol = rect->left()/mTileSize;
    int minRow = rect->top() /mTileSize;
    int maxCol = (rect->right() +mTileSize-1)/mTileSize;
    int maxRow = (rect->bottom()+mTileSize-1)/mTileSize;
    QRect square( 0, 0, mTileSize, mTileSize );

//std::cout << "render " << minRow << "," << minCol << "-" << maxRow << "," << maxCol << std::endl;
    for( int row = minRow; row <= maxRow; ++row ) {
        square.moveTop( row*mTileSize );

        for( int col = minCol; col <= maxCol; ++col ) {
            square.moveLeft( col * mTileSize );

            TileType type = board->tileAt( col, row );
            const QPixmap* pixmap = getPixmap( type );
            if ( !pixmap->isNull() ) {
                painter->drawPixmap( square, *pixmap );
            } else {
                int angle = 0;
                switch( type ) {
                case WATER:
                    painter->fillRect( square, QColor(33,33,255) );
                    break;
                case STONE_MIRROR__90:
                    pixmap = getPixmap( STONE_MIRROR );
                    angle = 90;
                    break;
                case STONE_MIRROR_180:
                    pixmap = getPixmap( STONE_MIRROR );
                    angle = 180;
                    break;
                case STONE_MIRROR_270:
                    pixmap = getPixmap( STONE_MIRROR );
                    angle = 270;
                    break;
                case STONE_SLIT_90:
                    pixmap = getPixmap( STONE_SLIT );
                    angle = 90;
                    break;
                case WOOD_DAMAGED:
                    BoardRenderer::renderPixmap( square, WOOD,   painter );
                    BoardRenderer::renderPixmap( square, DAMAGE, painter );
                    break;
                default: // EMPTY
                    painter->fillRect( square, Qt::black );
                    break;
                }
                if ( angle && pixmap ) {
                    renderRotatedPixmap( pixmap, square, angle, painter );
                }
            }
        }
    }

    const PieceSet& tiles = board->getPieceManager().getPieces();
    SimplePiece pos(MOVE, minRow, minCol);
    for( PieceSet::iterator iterator = tiles.lower_bound( &pos ); iterator != tiles.end(); ++iterator ) {
        if ( !(*iterator)->render( rect, *this, painter ) ) {
            break;
        }
    }
}

void BoardRenderer::renderPixmap( QRect& square, unsigned type, QPainter* painter )
{
    if ( const QPixmap* pixmap = getPixmap(type) ) {
        painter->drawPixmap( square, *pixmap );
    } else {
        std::cout << "*** attempt to paint unlisted pixmap " << type << std::endl;
    }
}


void BoardRenderer::renderRotation( QRect& square, int angle, QPainter* painter )
{
    // Note avoiding using square.center() here as it does some unexpected rounding:
    QPoint center(  square.left() + square.width()/2, square.top() + square.height()/2 );
    painter->translate(center);
    painter->rotate(angle);
    painter->translate(-center.x(), -center.y());
}

void BoardRenderer::renderRotatedPixmap( const QPixmap* pixmap, QRect& square, int angle, QPainter* painter )
{
    if ( angle ) {
        BoardRenderer::renderRotation( square, angle, painter );
    }
    painter->drawPixmap( square, *pixmap );
    painter->resetTransform();
}

void BoardRenderer::renderPiece( PieceType type, QRect& square, int angle, QPainter* painter )
{
    const QPixmap* pixmap = getPixmap( type );
    if ( pixmap->isNull() ) {
        std::cout << "no pixmap for " << type << std::endl;
        return;
    }

    renderRotatedPixmap( pixmap, square, angle, painter );
}

void BoardRenderer::renderInitialTank( Board* board, QPainter* painter )
{
    QRect square;
    BoardRenderer::renderPixmap( *getBounds( board->getTankStartPoint(), &square ), TANK, painter );
}

QRect* BoardRenderer::getBounds( const ModelPoint& forPoint, QRect* rect ) const
{
    rect->setRect( forPoint.mCol*mTileSize, forPoint.mRow*mTileSize, mTileSize, mTileSize );
    return rect;
}

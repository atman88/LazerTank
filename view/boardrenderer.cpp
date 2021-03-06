#include <iostream>
#include <QRect>
#include <QPainter>

#include "boardrenderer.h"
#include "controller/gameregistry.h"
#include "controller/game.h"
#include "controller/movecontroller.h"
#include "model/board.h"
#include "util/imageutils.h"

const QPoint BoardRenderer::NullPoint = QPoint(-1,-1);

BoardRenderer::BoardRenderer( int tileSize ) : mTileSize(tileSize), mPushIdDelineation(-1)
{
}

int BoardRenderer::tileSize() const
{
    return mTileSize;
}

void BoardRenderer::render( const QRect& rect, Board* board, QPainter* painter ) const
{
    int minCol = rect.left()  / mTileSize;
    int minRow = rect.top()   / mTileSize;
    int maxCol = rect.right() / mTileSize;
    int maxRow = rect.bottom()/ mTileSize;
    QRect square( 0, 0, mTileSize, mTileSize );

    for( int row = minRow; row <= maxRow; ++row ) {
        square.moveTop( row*mTileSize );

        for( int col = minCol; col <= maxCol; ++col ) {
            square.moveLeft( col * mTileSize );

            TileType type = board->tileAt( ModelPoint(col,row) );
            const QPixmap* pixmap = ResourcePixmap::getPixmap( type );
            if ( !pixmap->isNull() ) {
                painter->drawPixmap( square, *pixmap );
            } else {
                int angle = 0;
                switch( type ) {
                case STONE_MIRROR__90:
                    pixmap = ResourcePixmap::getPixmap( STONE_MIRROR );
                    angle = 90;
                    break;
                case STONE_MIRROR_180:
                    pixmap = ResourcePixmap::getPixmap( STONE_MIRROR );
                    angle = 180;
                    break;
                case STONE_MIRROR_270:
                    pixmap = ResourcePixmap::getPixmap( STONE_MIRROR );
                    angle = 270;
                    break;
                case STONE_SLIT_90:
                    pixmap = ResourcePixmap::getPixmap( STONE_SLIT );
                    angle = 90;
                    break;
                case WOOD_DAMAGED:
                    BoardRenderer::renderPixmap( square, WOOD_DAMAGED, painter );
                    break;
                default: // EMPTY
                    break;
                }
                if ( angle && pixmap ) {
                    renderRotatedPixmap( pixmap, square, angle, painter );
                }
            }
        }
    }

    const PieceSet& pieces = board->getPieceManager().getPieces();
    SimplePiece pos(MOVE, minCol, minRow);
    for( auto iterator = pieces.lower_bound( &pos ); iterator != pieces.end(); ++iterator ) {
        if ( !(*iterator)->render( &rect, *this, painter ) ) {
            break;
        }
    }
}

void BoardRenderer::renderPixmap( QRect& square, unsigned type, QPainter* painter )
{
    if ( const QPixmap* pixmap = ResourcePixmap::getPixmap(type) ) {
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
    if ( !angle ) {
        painter->drawPixmap( square, *pixmap );
    } else {
        QTransform save = painter->transform();
        BoardRenderer::renderRotation( square, angle, painter );
        painter->drawPixmap( square, *pixmap );
        painter->setTransform( save );
    }
}

void BoardRenderer::renderPiece( PieceType type, QRect& square, int angle, QPainter* painter )
{
    const ResourcePixmap* pixmap = ResourcePixmap::getPixmap( type );
    if ( pixmap->isNull() ) {
        std::cout << "no pixmap for " << type << std::endl;
        return;
    }
    const QPixmap* pm;
    if ( pixmap->hasColorableTag() ) {
        pm = pixmap->getForColor( painter->pen().color() );
    } else {
        pm = pixmap;
    }

    renderRotatedPixmap( pm, square, angle, painter );
}

void BoardRenderer::renderListIn( PieceSet::iterator iterator, PieceSet::iterator end, const QRect* dirty, QPainter* painter )
{
    if ( mPushIdDelineation < 0 ) {
        while( iterator != end ) {
            if ( !(*iterator)->render( dirty, *this, painter ) ) {
                break;
            }
            ++iterator;
        }
    } else {
        bool usingAlternateColor = false;
        QPen savePen = painter->pen();

        while( iterator != end ) {
            if ( (*iterator)->getPushedId() <= mPushIdDelineation ) {
                if ( !usingAlternateColor ) {
                    painter->setPen( Qt::blue );
                    usingAlternateColor = true;
                }
            } else if ( usingAlternateColor ) {
                painter->setPen( savePen );
                usingAlternateColor = false;
            }
            if ( !(*iterator)->render( dirty, *this, painter ) ) {
                break;
            }
            ++iterator;
        }

        if ( usingAlternateColor ) {
            painter->setPen( savePen );
        }
    }
}

void BoardRenderer::setPushIdDelineation( int pushIdDelineation )
{
    mPushIdDelineation = pushIdDelineation;
}

void BoardRenderer::renderInitialTank( Board* board, QPainter* painter )
{
    QRect square;
    const ModelVector& v = board->getTankStartVector();
    BoardRenderer::renderPiece( TANK, *getBounds( v, &square ), v.mAngle, painter );
}

QRect* BoardRenderer::getBounds( const ModelPoint& forPoint, QRect* rect ) const
{
    rect->setRect( forPoint.mCol*mTileSize, forPoint.mRow*mTileSize, mTileSize, mTileSize );
    return rect;
}

void BoardRenderer::renderMoves( const QRect& rect, GameRegistry* registry, QPainter* painter )
{
    MoveController& moveController = registry->getMoveController();
    SimplePiece pos(MOVE, rect.left()/mTileSize, rect.top()/mTileSize);
    moveController.render( &pos, &rect, *this, painter );
    if ( !moveController.replaying() ) {
        if ( const PieceSet* deltas = registry->getGame().getDeltaPieces() ) {
            int pushIdDelineation = moveController.getPushIdDelineation();
            if ( pushIdDelineation < 0 ) {
                QPen savePen( painter->pen() );
                painter->setPen( Qt::blue );
                renderListIn( deltas->lower_bound( &pos ), deltas->end(), &rect, painter );
                painter->setPen( savePen );
            } else {
                setPushIdDelineation( pushIdDelineation );
                renderListIn( deltas->lower_bound( &pos ), deltas->end(), &rect, painter );
                setPushIdDelineation( -1 );
            }
        }
    }
}

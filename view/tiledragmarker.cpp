#include <QPainter>

#include "tiledragmarker.h"
#include "model/board.h"

#define MARKER_HEIGHT (mTileSize/2-2)
#define MARKER_WIDTH  (mTileSize-4)

TileDragMarker::TileDragMarker( QObject* parent ) : QObject(parent), mAngleMask(0), mFocusAngle(-1), mTileSize(0)
{
}

void TileDragMarker::render( const QRect* rect, QPainter* painter )
{
    if ( mAngleMask && rect->intersects( mBounds ) ) {
        const QPen& savedPen = painter->pen();
        QPen pen( Qt::green );
        pen.setWidth(2);
        painter->setPen(pen);

        QPainterPath path( { 0, 0 } );
        path.lineTo( -MARKER_WIDTH/2, -MARKER_HEIGHT );
        path.lineTo( MARKER_WIDTH/2,  -MARKER_HEIGHT );
        path.lineTo( 0, 0 );

        for( int mask = 1<<4, angle = 270; (mask >>= 1); angle -= 90 ) {
            if ( mAngleMask & mask ) {
                QTransform save = painter->transform();
                painter->translate(mCenter);
                painter->rotate( angle );
                // center at the arrow tip:
                painter->translate( 0, mTileSize/2-1 + MARKER_HEIGHT );
                if ( angle == mFocusAngle ) {
                    painter->fillPath( path, Qt::green );
                } else {
                    painter->drawPath( path );
                }
                painter->setTransform( save );
            }
        }

        painter->setPen( savedPen );
    }
}

void TileDragMarker::enable( unsigned angleMask, QPoint center, int tileSize )
{
    if ( !(angleMask & 0xf) ) {
        disable();
        return;
    }
    if ( !mBounds.isNull() ) {
        emit rectDirty( mBounds );
    }

    mCenter = center;
    mTileSize = tileSize;
    mAngleMask = angleMask;

    mBounds = QRect( mCenter.x()-mTileSize/2, mCenter.y()-mTileSize/2, mTileSize, mTileSize );
    if ( mAngleMask & (1<<0) ) mBounds.setTop(    mBounds.top()    - MARKER_HEIGHT );
    if ( mAngleMask & (1<<1) ) mBounds.setLeft(   mBounds.left()   - MARKER_HEIGHT );
    if ( mAngleMask & (1<<2) ) mBounds.setBottom( mBounds.bottom() + MARKER_HEIGHT );
    if ( mAngleMask & (1<<3) ) mBounds.setRight(  mBounds.right()  + MARKER_HEIGHT );
    emit rectDirty( mBounds );
}

void TileDragMarker::disable()
{
    if ( mAngleMask ) {
        emit rectDirty( mBounds );
        mAngleMask = 0;
        mCenter = QPoint();
        mBounds = QRect();
    }
}

void TileDragMarker::setFocus( int angle )
{
    bool changed = false;
    if ( angle >= 0 && !(mAngleMask & (1 << (angle / 90)) ) ) {
        changed = (mFocusAngle >= 0);
        mFocusAngle = -1;
    } else if ( angle != mFocusAngle ) {
        mFocusAngle = angle;
        changed = true;
    }

    if ( changed ) {
        emit rectDirty( mBounds );
    }
}

void TileDragMarker::setEntryFocus( int entryAngle )
{
    setFocus( (entryAngle + 180) % 360 );
}

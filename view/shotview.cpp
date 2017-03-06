#include <iostream>
#include <QPainterPath>
#include "shotview.h"
#include "model/board.h"
#include "controller/game.h"
#include "util/renderutils.h"

// Define our own Null value because QPoint's isNull method keys on (0,0) which is a valid model point
static const QPoint NullPoint(-1,-1);

ShotView::ShotView(QObject *parent) : QObject(parent), mShooter(0), mTerminationAngle(-1), mKillTheTank(false)
{
    mLeadPoint = mTailPoint = NullPoint;
    mPen.setWidth( 2 );
}

void ShotView::reset()
{
    mKillTheTank = false;
    mTerminationAngle = -1;
    mLeadPoint = mTailPoint = NullPoint;
    mShooter = 0;
    mBendPoints.clear();
}

QPoint toStartPoint( int x, int y, int angle )
{
    switch( angle ) {
    case   0: x += 24/2;            break;
    case  90: x +=   24; y += 24/2; break;
    case 180: x += 24/2; y +=   24; break;
    case 270:            y += 24/2; break;
    }
    return QPoint(x, y);
}

QPoint ShotView::getStartPoint()
{
    if ( mShooter ) {
        return toStartPoint( mShooter->getX().toInt(), mShooter->getY().toInt(), mShooter->getRotation().toInt() );
    }
    return mTailPoint;
}

void ShotView::render( QPainter* painter )
{
    QPoint startPoint = getStartPoint();
    if ( startPoint != NullPoint ) {
        if ( mLeadPoint != NullPoint ) {
            QPainterPath painterPath;
            painterPath.moveTo( startPoint );

            for( auto it : mBendPoints ) {
                painterPath.lineTo( it );
            }

            painterPath.lineTo( mLeadPoint );

            painter->setPen( mPen );
            painter->drawPath( painterPath );
        }

        if ( mTerminationAngle >= 0 ) {
            int cx = mLeadPoint.x();
            int cy = mLeadPoint.y();

            if ( mTerminationAngle ) {
                painter->translate(cx, cy);
                painter->rotate(mTerminationAngle);
                painter->translate(-cx, -cy);
            }

            // draw a splat:
            --cy;
            QPen pen( mPen );
            QColor color = pen.color();
            int alpha = color.alpha();
            for( int i = 1; i <= 4; ++i ) {
                painter->drawLine( cx-(i*2)-2,   cy, cx-(i*2),   cy );
                painter->drawLine( cx+(i*2)-1, cy, cx+(i*2)+2+1, cy );
                painter->drawPoint( cx-i, cy-i );
                painter->drawPoint( cx+i, cy-i );

                alpha = (alpha >> 1 ) + (alpha >> 2);
                color.setAlpha( alpha );
                pen.setColor( color );
                painter->setPen( pen );
            }
            painter->resetTransform();
        }
    }
}

bool ShotView::hasTerminationPoint()
{
    return mTerminationAngle >= 0;
}

QPoint modelToViewPoint( int col, int row )
{
    return QPoint( col*24+24/2, row*24+24/2 );
}

void ShotView::commenceFire( Shooter* shooter )
{
    mShooter = shooter;
    mLeadPoint = toStartPoint( shooter->getX().toInt(), shooter->getY().toInt(), shooter->getRotation().toInt() );
}

void ShotView::emitDirtySegment( QPoint p1, QPoint p2 )
{
    int x1 = min( p1.x(), p2.x() )-1;
    int y1 = min( p1.y(), p2.y() )-1;
    int x2 = max( p1.x(), p2.x() )+1;
    int y2 = max( p1.y(), p2.y() )+1;
    QRect rect( x1, y1, x2, y2 );
    emit rectDirty( rect );
}

void ShotView::grow( int col, int row, int startAngle, int endAngle )
{
    QPoint startPoint( mLeadPoint );
    mLeadPoint = modelToViewPoint( col, row );

    if ( startAngle != endAngle ) {
        switch( startAngle ) {
        case   0:
        case 180: mBendPoints.push_back( QPoint( startPoint.x(), mLeadPoint.y()  ) ); break;
        case  90:
        case 270: mBendPoints.push_back( QPoint( mLeadPoint.x(),  startPoint.y() ) ); break;
        default:
            break;
        }
    }

    emitDirtySegment( startPoint, mLeadPoint );
}

void ShotView::emitSplatDirty()
{
    // dirty an area large enough to cover the the splat
    QRect rect( mLeadPoint.x()-24/2, mLeadPoint.y()-24/2, 24, 24 );
    emit rectDirty( rect );
}

void ShotView::addTermination( int endAngle, int endOffset )
{
    mTerminationAngle = endAngle;

    int x = mLeadPoint.x();
    int y = mLeadPoint.y();

    int centerAdjust = (mLeadPoint != getStartPoint() ? 24/2 : 0);
    switch( endAngle ) {
    case   0: y += endOffset - centerAdjust; break;
    case  90: x += endOffset + centerAdjust; break;
    case 180: y += endOffset + centerAdjust; break;
    case 270: x += endOffset - centerAdjust; break;
    }

    emitDirtySegment( mLeadPoint, QPoint(x,y) );

    mLeadPoint.setX( x );
    mLeadPoint.setY( y );
    emitSplatDirty();
}

bool ShotView::trimToward( QPoint target )
{
    if ( target.x() == mTailPoint.x() ) {
        int dy = mTailPoint.y()-target.y();
        if ( dy > 24 ) {
            mTailPoint.setY( mTailPoint.y() - 24 );
            return true;
        }
        if ( dy < -24 ) {
            mTailPoint.setY( mTailPoint.y() + 24 );
            return true;
        }
        return false;
    }

    int dx = mTailPoint.x() - target.x();
    if ( dx > 24 ) {
        mTailPoint.setX( mTailPoint.x() - 24 );
        return true;
    }
    if ( dx < -24 ) {
        mTailPoint.setX( mTailPoint.x() + 24 );
        return true;
    }
    return false;
}

bool ShotView::shedTail()
{
    if ( mShooter ) {
        mTailPoint = toStartPoint( mShooter->getX().toInt(), mShooter->getY().toInt(), mShooter->getRotation().toInt() );
        mShooter = 0;
    }

    if ( mTailPoint == mLeadPoint ) {
        emitSplatDirty();
        mTailPoint = mLeadPoint = NullPoint;
    } else {
        QPoint startTailPoint( mTailPoint );

        if ( !mBendPoints.empty() ) {
            if ( !trimToward( mBendPoints.front() ) ) {
                mTailPoint = mBendPoints.front();
                mBendPoints.pop_front();
            }
        } else if ( !trimToward( mLeadPoint ) ) {
            mTailPoint = mLeadPoint;
        }

        emitDirtySegment( startTailPoint, mTailPoint );
        return true;
    }

    return false;
}

void ShotView::killTheTank()
{
    QPoint p = mLeadPoint;
    if ( p == NullPoint ) {
        p = mTailPoint;
    }
    mKillTheTank = true;
}

void ShotView::setColor(QColor color)
{
    mPen.setColor( color );
}

#include <iostream>
#include <QPainterPath>
#include "shotview.h"
#include "model/board.h"
#include "controller/game.h"
#include "util/renderutils.h"

using namespace std;

ShotView::ShotView(QObject *parent) : QObject(parent), mShooter(0), mTerminationAngle(-1), mKillTheTank(false)
{
    mLeadPoint = mTailPoint = NullPoint;
    mPen.setWidth( 2 );
}

void ShotView::reset()
{
    // dirty it if visible to erase
    QPoint curPoint = getStartPoint();
    if ( curPoint != NullPoint ) {
        if ( mLeadPoint != NullPoint ) {
            for( auto it : mBendPoints ) {
                emitDirtySegment( curPoint, it );
                curPoint = it;
            }
            emitDirtySegment( curPoint, mLeadPoint );
        }

        if ( mTerminationAngle >= 0 ) {
            emitSplatDirty();
        }
    }

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
    if ( mShooter && mTailPoint == NullPoint ) {
        return toStartPoint( mShooter->getViewX().toInt(), mShooter->getViewY().toInt(), mShooter->getViewRotation().toInt() % 360 );
    }
    return mTailPoint;
}

Shooter* ShotView::getShooter() const
{
    return mShooter;
}

QPoint ShotView::getLeadPoint() const
{
    return mLeadPoint;
}

void ShotView::render( QPainter* painter )
{
    QPoint startPoint = getStartPoint();
    if ( startPoint != NullPoint ) {
        painter->setPen( mPen );

        if ( mLeadPoint != NullPoint ) {
            QPainterPath painterPath;
            painterPath.moveTo( startPoint );

            for( auto it : mBendPoints ) {
                painterPath.lineTo( it );
            }

            painterPath.lineTo( mLeadPoint );

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

void ShotView::commenceFire( Shooter* shooter )
{
    mShooter = shooter;
    mTailPoint = NullPoint;
    mLeadAngle = shooter->getViewRotation().toInt() % 360;
    mLeadPoint = toStartPoint( shooter->getViewX().toInt(), shooter->getViewY().toInt(), mLeadAngle );
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

void ShotView::grow( QPoint squareCenterPoint, int direction )
{
    QPoint startPoint = mLeadPoint;
    if ( direction != mLeadAngle ) {
        mBendPoints.push_back( startPoint );
        mLeadAngle = direction;
    }
    mLeadPoint = squareCenterPoint;
    emitDirtySegment( startPoint, mLeadPoint );
}

void ShotView::emitSplatDirty()
{
    // dirty an area large enough to cover the the splat
    QRect rect( mLeadPoint.x()-24/2, mLeadPoint.y()-24/2, 24, 24 );
    emit rectDirty( rect );
}

void ShotView::addTermination( int endAngle, QPoint& hitPoint )
{
    mTerminationAngle = endAngle;

    if ( hitPoint != mLeadPoint ) {
        if ( mLeadAngle != endAngle ) {
            mBendPoints.push_back( mLeadPoint );
            mLeadAngle = endAngle;
        }
        emitDirtySegment( mLeadPoint, hitPoint );
        mLeadPoint = hitPoint;
    }
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
    if ( mTailPoint == NullPoint ) {
        mTailPoint = toStartPoint( mShooter->getViewX().toInt(), mShooter->getViewY().toInt(), mShooter->getViewRotation().toInt() % 360 );
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

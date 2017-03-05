#include <iostream>
#include <QPainterPath>
#include "shotview.h"
#include "model/board.h"
#include "controller/Game.h"
#include "util/renderutils.h"

ShotView::ShotView(QObject *parent) : QObject(parent), mKillShowing(false), mEndAngle(-1), mShooter(0)
{
    mPen.setWidth( 2 );
}

void ShotView::reset()
{
    mKillShowing = false;
    mEndAngle = -1;
    mEndPoint = QPoint();
    mTailPoint = QPoint();
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

void ShotView::render( const QRect* rect, QPainter* painter )
{
    if ( !mEndPoint.isNull() ) {
        QPainterPath painterPath;
        painterPath.moveTo( getStartPoint() );

        for( auto it : mBendPoints ) {
            painterPath.lineTo( it );
        }
        painterPath.lineTo( mEndPoint );
        painter->setPen( mPen );
        painter->drawPath( painterPath );

        if ( mEndAngle >= 0 ) {
            int cx = mEndPoint.x();
            int cy = mEndPoint.y();

            if ( mEndAngle ) {
                painter->translate(cx, cy);
                painter->rotate(mEndAngle);
                painter->translate(-cx, -cy);
            }

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

bool ShotView::hasEndPoint()
{
    return mEndAngle >= 0;
}

QPoint modelToViewPoint( int col, int row )
{
    return QPoint( col*24+24/2, row*24+24/2 );
}

void ShotView::commenceFire( Shooter* shooter )
{
    mShooter = shooter;
    mEndPoint = toStartPoint( shooter->getX().toInt(), shooter->getY().toInt(), shooter->getRotation().toInt() );
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
    QPoint startPoint( mEndPoint );
    mEndPoint = modelToViewPoint( col, row );

    if ( startAngle != endAngle ) {
        switch( startAngle ) {
        case   0:
        case 180: mBendPoints.push_back( QPoint( startPoint.x(), mEndPoint.y()  ) ); break;
        case  90:
        case 270: mBendPoints.push_back( QPoint( mEndPoint.x(),  startPoint.y() ) ); break;
        default:
            break;
        }
    }

    emitDirtySegment( startPoint, mEndPoint );
}

void ShotView::growEnd( int endAngle, int endOffset )
{
    mEndAngle = endAngle;

    int x = mEndPoint.x();
    int y = mEndPoint.y();

    int centerAdjust = (mEndPoint != getStartPoint() ? 24/2 : 0);
    switch( endAngle ) {
    case   0: y += endOffset - centerAdjust; break;
    case  90: x += endOffset + centerAdjust; break;
    case 180: y += endOffset + centerAdjust; break;
    case 270: x += endOffset - centerAdjust; break;
    }

    mEndPoint.setX( x );
    mEndPoint.setY( y );

    QRect rect( x-24/2, y-24/2, 24, 24 );
    emit rectDirty( rect );
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

    int dx = mTailPoint.x() - mTailPoint.x();
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

void ShotView::shedTail()
{
    if ( mShooter ) {
        mTailPoint = toStartPoint( mShooter->getX().toInt(), mShooter->getY().toInt(), mShooter->getRotation().toInt() );
        mShooter = 0;
    } else {
        if ( mTailPoint == mEndPoint ) {
            QRect rect( mEndPoint.x()-24/2, mEndPoint.y()-24/2, 24, 24 );
            emit rectDirty( rect );
            mEndPoint = mTailPoint = QPoint(); // stop commencing
        } else {
            QPoint startTailPoint( mTailPoint );

            if ( !mBendPoints.empty() ) {
                if ( !trimToward( mBendPoints.front() ) ) {
                    mTailPoint = mBendPoints.front();
                    mBendPoints.pop_front();
                }
            } else if ( !trimToward( mEndPoint ) ) {
                mTailPoint = mEndPoint;
            }

            emitDirtySegment( startTailPoint, mTailPoint );
        }
    }
}

void ShotView::showKill()
{
    mKillShowing = true;
}

bool ShotView::commencing()
{
    return !mEndPoint.isNull();
}

void ShotView::setColor(QColor color)
{
    mPen.setColor( color );
}

QColor ShotView::getColor() const
{
    return mPen.color();
}

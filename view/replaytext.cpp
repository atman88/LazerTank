#include <iostream>
#include <QWindow>

#include "replaytext.h"
#include "boardwindow.h"
#include "controller/game.h"
#include "controller/movecontroller.h"
#include "util/gameutils.h"

ReplayText::ReplayText( QObject* parent, const QString& text, int minAlpha, int maxAlpha ) : QObject(parent), mText(text),
  mMinAlpha(minAlpha), mMaxAlpha(maxAlpha), mInitialized(false), mEnabled(false)
{
}

void ReplayText::render( const QRect* rect, QPainter* painter )
{
    if ( mBounds.isNull() ) {
        if ( GameRegistry* registry = getRegistry(this) ) {
            // lazily initialize on first use:
            if ( !mInitialized ) {
                mFont = painter->font();
                mFont.setBold( true );
                mFont.setItalic( true );
                mFont.setPixelSize( 36 );
                mPen.setColor( QColor(255,255,255,mMaxAlpha) );

                mAnimation.setTargetObject(this);
                mAnimation.setPropertyName("alpha");
                mAnimation.setEasingCurve( QEasingCurve::InOutQuad );
                mAnimation.setDuration( 2000 );

                QObject::connect( &registry->getMoveController(), &MoveController::replayFinished, this, &ReplayText::disable );
                QObject::connect( &mAnimation, &QPropertyAnimation::finished, this, &ReplayText::startCycle, Qt::QueuedConnection );
                mInitialized = true;
            }

            if ( BoardWindow* window = registry->getWindow() ) {
                QSize size = window->size();
                painter->setFont( mFont );
                mBounds = painter->boundingRect( QRect(QPoint(0,0),size), Qt::AlignCenter, mText );
            }
        }
    }

    if ( !mEnabled ) {
        mEnabled = true;
        startCycle();
    }

    if ( rect->intersects( mBounds ) ) {
        painter->setFont( mFont );
        painter->setPen( mPen );
        painter->drawText( mBounds, mText );
    }
}

void ReplayText::startCycle()
{
    if ( mEnabled ) {
        int alpha = mPen.color().alpha();
        mAnimation.setStartValue( alpha );
        mAnimation.setEndValue( alpha == mMaxAlpha ? mMinAlpha : mMaxAlpha );
        mAnimation.start();
    }
}

void ReplayText::disable()
{
    mEnabled = false;
    mAnimation.stop();
    if ( !mBounds.isNull() ) {
        emit dirty( mBounds );
    }
}

QVariant ReplayText::getAlpha() const
{
    return mPen.color().alpha();
}

void ReplayText::setAlpha( QVariant& vAlpha )
{
    int alpha = vAlpha.toInt();
    if ( alpha != mPen.color().alpha() ) {
        QColor color = mPen.color();
        color.setAlpha( alpha );
        mPen.setColor( color );
        if ( !mBounds.isNull() ) {
            emit dirty( mBounds );
        }
    }
}

void ReplayText::onResize()
{
    // nullify the bounds rectangle to recompute on next use
    mBounds = QRect();
}

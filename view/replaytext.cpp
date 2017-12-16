#include <iostream>
#include <QWindow>

#include "replaytext.h"
#include "boardwindow.h"
#include "controller/game.h"
#include "controller/movecontroller.h"
#include "util/gameutils.h"

ReplayText::ReplayText( QWidget* parent, const QString& text, int minAlpha, int maxAlpha ) : WhatsThisAwareLabel(parent),
  mMinAlpha(minAlpha), mMaxAlpha(maxAlpha)
{
    setText( text );
    setAlignment( Qt::AlignHCenter );

    mPalette = palette();
    mPalette.setColor( QPalette::WindowText, QColor(255,255,255,mMaxAlpha) );
    setPalette( mPalette );

    QFont myFont( font() );
    myFont.setBold( true );
    myFont.setItalic( true );
    setFont( myFont );

    mAnimation.setTargetObject(this);
    mAnimation.setPropertyName("alpha");
    mAnimation.setEasingCurve( QEasingCurve::InOutQuad );
    mAnimation.setDuration( 2000 );

    QObject::connect( &mAnimation, &QPropertyAnimation::finished, this, &ReplayText::startCycle, Qt::QueuedConnection );
}

void ReplayText::startCycle()
{
    if ( isVisible() ) {
        int alpha = mPalette.color(QPalette::WindowText).alpha();
        mAnimation.setStartValue( alpha );
        mAnimation.setEndValue( alpha == mMaxAlpha ? mMinAlpha : mMaxAlpha );
        mAnimation.start();
    }
}

void ReplayText::mousePressEvent( QMouseEvent* event )
{
    if ( event->button() == Qt::LeftButton ) {
        if ( GameRegistry* registry = getRegistry(this) ) {
            checkForReplay( registry );
        }
        return;
    }
    WhatsThisAwareLabel::mousePressEvent( event );
}

QVariant ReplayText::getAlpha() const
{
    return mPalette.color(QPalette::WindowText).alpha();
}

void ReplayText::setAlpha( QVariant& vAlpha )
{
    int alpha = vAlpha.toInt();
    const QColor& color = mPalette.color( QPalette::WindowText );
    if ( alpha != color.alpha() ) {
        mPalette.setColor( QPalette::WindowText, QColor(255,255,255,alpha) );
        setPalette( mPalette );
    }
}

void ReplayText::showEvent( QShowEvent* )
{
    startCycle();
}

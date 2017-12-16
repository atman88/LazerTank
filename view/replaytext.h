#ifndef REPLAYTEXT_H
#define REPLAYTEXT_H

#include <QObject>
#include <QRect>
#include <QPen>
#include <QFont>
#include <QString>
#include <QPropertyAnimation>

#include "whatsthisaware.h"


class ReplayText : public WhatsThisAwareLabel
{
    Q_OBJECT
    Q_PROPERTY(QVariant alpha READ getAlpha WRITE setAlpha)

public:
    ReplayText( QWidget* parent, const QString& text, int minAlpha = 64, int maxAlpha = 255 );

    QVariant getAlpha() const;

    /**
     * @brief Modifies this object's color alpha value
     * @param alpha A value between 0 and 255
     */
    void setAlpha( QVariant& alpha );

protected:
    void showEvent( QShowEvent* ) override;

private slots:
    /**
     * @brief Kick off the animation for one rise or fall
     */
    void startCycle();

private:
    QPropertyAnimation mAnimation;
    int mMinAlpha;
    int mMaxAlpha;
    QPalette mPalette;

protected:
    void mousePressEvent(QMouseEvent *event) override;
};

#endif // REPLAYTEXT_H

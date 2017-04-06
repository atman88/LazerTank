#ifndef REPLAYTEXT_H
#define REPLAYTEXT_H

#include <QObject>
#include <QRect>
#include <QPen>
#include <QFont>
#include <QString>
#include <QPropertyAnimation>

class ReplayText : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant alpha READ getAlpha WRITE setAlpha)

public:
    ReplayText( QObject* parent, const QString& text, int minAlpha = 24, int maxAlpha = 96 );
    void render( const QRect* rect, QPainter* painter );

    QVariant getAlpha() const;

signals:
    void dirty( QRect& rect );

public slots:\
    /**
     * @brief Notifies that its window size has changed
     */
    void onResize();

    /**
     * @brief Return this object to its dormant state
     */
    void disable();

    /**
     * @brief Modifies this object's color alpha value
     * @param alpha A value between 0 and 255
     */
    void setAlpha( QVariant& alpha );

private slots:
    /**
     * @brief Kick off the animation for one rise or fall
     */
    void startCycle();

private:
    QString mText;
    QRect mBounds;
    QPen mPen;
    QFont mFont;
    QPropertyAnimation mAnimation;
    int mMinAlpha;
    int mMaxAlpha;
    bool mInitialized;
    bool mEnabled;
};

#endif // REPLAYTEXT_H

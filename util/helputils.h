#ifndef HELPUTILS_H
#define HELPUTILS_H

#include <QPoint>
#include <QEvent>

class GameRegistry;

class QltWhatsThisEvent : public QEvent
{
public:
    QltWhatsThisEvent( QPoint pos, QString& text ) : QEvent(getEventType()), mPos(pos), mText(text)
    {
    }

    static QEvent::Type getEventType();
    QPoint& getPos();
    QString& getHelpText();

private:
    QPoint mPos;
    QString mText;
};

void whatsthis( QPoint* pos, unsigned what, GameRegistry* registry, QObject* receiver );

#endif // HELPUTILS_H

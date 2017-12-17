#ifndef HELPUTILS_H
#define HELPUTILS_H

#include <QPoint>
#include <QEvent>

class GameRegistry;

/**
 * @brief Notifies the foreground that a what's this query is ready
 */
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

/**
 * @brief Process a what's this request in the background
 * @param pos Screen point associated with the request. Typically the mouse pointer.
 * @param what The tile type or piece type to obtain text for
 * @param registry Reference to the game registry
 * @param receiver Object to receive the resultant QltWhatsThisEvent
 */
void whatsthis( QPoint* pos, unsigned what, GameRegistry* registry, QObject* receiver );

#endif // HELPUTILS_H

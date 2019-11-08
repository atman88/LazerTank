#ifndef WHATTHISAWARE_H
#define WHATTHISAWARE_H

#include <QLabel>
#include <QAction>

QT_FORWARD_DECLARE_CLASS(QMouseEvent)

class WhatsThisAwareLabel : public QLabel
{
public:
    WhatsThisAwareLabel( QWidget* parent = nullptr );

protected:
    void mousePressEvent( QMouseEvent* event ) override;

private:
    QAction mAction;
};

#endif // WHATTHISAWARE_H

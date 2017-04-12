#ifndef GAMEINITIALIZER_H
#define GAMEINITIALIZER_H

#include <QObject>

class GameInitializer : public QObject
{
    Q_OBJECT
public:
    explicit GameInitializer(QObject *parent = 0);

signals:

public slots:
};

#endif // GAMEINITIALIZER_H
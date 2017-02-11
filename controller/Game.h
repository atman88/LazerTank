#ifndef GAMECONTROLLER_H
#define GAMECONTROLLER_H

#include <QObject>
#include <list>

#include "model/board.h"
#include "model/intent.h"

struct GameHandle
{
    class Game* game;
};
Q_DECLARE_METATYPE(GameHandle)

typedef std::list<Intent> IntentList;
Q_DECLARE_METATYPE(IntentList)

class Game : public QObject
{
    Q_OBJECT

public:
    Game( Board* board );
    GameHandle getHandle();
    Board* getBoard();
    int getTankX();
    int getTankY();
    bool canMoveFrom( int angle, int *x, int *y );
    bool addMove( int angle );
    bool getAdjacentPosition( int angle, int *x, int *y );
    bool canPlaceAt( int x, int y );

signals:
    void intentAdded( const Intent& );
    void intentRemoved( const Intent& );
    void tankInitialized( int x, int y );

public slots:
    void clearIntents();
    void onTankMoved( int x, int y );

private:
    void addMoveInternal( int angle, int x, int y );
    GameHandle mHandle;
    Board* mBoard;
    int mTankX;
    int mTankY;
    std::list<Intent> mIntents;
};

#endif // GAMECONTROLLER_H

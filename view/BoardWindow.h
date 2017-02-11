#ifndef BOARDWINDOW_H
#define BOARDWINDOW_H

#include <QtGui>
#include "tank.h"
#include "controller/Game.h"

class BoardWindow : public QWindow
{
    Q_OBJECT
    Q_PROPERTY(GameHandle game READ getGame WRITE setGame)

public:
    explicit BoardWindow(QWindow *parent = 0);
    ~BoardWindow()
    {
        delete mTank;
    }

    GameHandle getGame() const;
    virtual void render(QRegion *region);

signals:

public slots:
    void setGame( const GameHandle game );
    void renderLater(QRect* region);
    void renderNow();
    void onTankChanged( QRect rect );
    void onTankStopped();
    void renderIntentLater( const Intent& intent );
    void eraseIntent( const Intent& );

protected:
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent *ev) override;
    void keyReleaseEvent(QKeyEvent *ev) override;

    void resizeEvent(QResizeEvent *event) override;
    void exposeEvent(QExposeEvent *event) override;

private:
    void renderMove( int x, int y, int angle = 0 );
    void renderIntent(const Intent& , QPainter *painter);

    Tank* mTank;

    QBackingStore *mBackingStore;
    QRegion *mDirtyRegion;

    Game* mGame;
    QPixmap mStonePixmap;
    QPixmap mDirtPixmap;
    QPixmap mMoveIndicatorPixmap;
    int mActiveMoveDirection;
};

#endif // BOARDWINDOW_H

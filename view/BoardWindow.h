#ifndef BOARDWINDOW_H
#define BOARDWINDOW_H

#include <QtGui>
#include "tank.h"
#include "shot.h"
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

    void renderLater(QRect* region);

signals:

public slots:
    void setGame( const GameHandle game );
    void renderRectLater( QRect rect );
    void renderNow();
    void onTankChanged( QRect rect );
    void onTankStopped();
    void renderPieceLater( const Piece& piece );
    void onPieceStopped();

protected:
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent *ev) override;
    void keyReleaseEvent(QKeyEvent *ev) override;

    void resizeEvent(QResizeEvent *event) override;
    void exposeEvent(QExposeEvent *event) override;

private:
    void renderMove( int x, int y, int angle = 0 );
    void renderPiece( PieceType type, int x, int y, int angle, QPainter *painter );
    void renderListAt(QPainter* painter, PieceSet::iterator *iterator, PieceSet::iterator end, Piece &pos );

    Tank* mTank;
    Shot* mShot;

    QBackingStore *mBackingStore;
    QRegion *mDirtyRegion;

    Game* mGame;
    QPixmap mStonePixmap;
    QPixmap mDirtPixmap;
    QPixmap mFlagPixmap;
    QPixmap mTilePixmap;
    QPixmap mMoveIndicatorPixmap;
    QPixmap mShotStraightPixmap;

    int mActiveMoveDirection;
};

#endif // BOARDWINDOW_H

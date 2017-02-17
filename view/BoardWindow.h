#ifndef BOARDWINDOW_H
#define BOARDWINDOW_H

#include <QtGui>
#include <QMouseEvent>
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

signals:

public slots:
    void setGame( const GameHandle game );
    void renderLater(const QRect &rect);
    void renderNow();
    void renderPieceLater( const Piece& piece );
    void onBoardLoaded();

protected:
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent *ev) override;
    void keyReleaseEvent(QKeyEvent *ev) override;
    void mousePressEvent( QMouseEvent* event ) override;

    void resizeEvent(QResizeEvent *event) override;
    void exposeEvent(QExposeEvent *event) override;

private:
    void renderRotatedPixmap(QPixmap& pixmap, int x, int y, int angle, QPainter &painter );
    void renderMove( int x, int y, int angle = 0 );
    void renderPiece(PieceType type, int x, int y, int angle, QPainter &painter );
    void renderListAt(PieceSet::iterator *iterator, PieceSet::iterator end, Piece& pos, QPainter& painter );

    Tank* mTank;
    Shot* mShot;

    QBackingStore *mBackingStore;
    QRegion *mDirtyRegion;

    Game* mGame;
    QPixmap mStonePixmap;
    QPixmap mDirtPixmap;
    QPixmap mTileSunkPixmap;
    QPixmap mFlagPixmap;
    QPixmap mTilePixmap;
    QPixmap mMoveIndicatorPixmap;
    QPixmap mShotStraightPixmap;
    QPixmap mShotRightPixmap;
    QPixmap mWallMirrorPixmap;
    QPixmap mStoneSlitPixmap;
    QPixmap mWoodPixmap;
    QPixmap mWoodDamaged;

    int mActiveMoveDirection;
};

#endif // BOARDWINDOW_H

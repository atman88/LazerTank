#ifndef BOARDWINDOW_H
#define BOARDWINDOW_H

class BoardWindow;

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
        delete mBackingStore;
    }

    GameHandle getGame() const;
    virtual void render(QRegion *region);
    Tank* getTank();
    void onTankKilled();

signals:

public slots:
    void setGame( const GameHandle game );
    void renderLater(const QRect &rect);
    void renderNow();
    void renderSquareLater( int col, int row );
    void onBoardLoaded();

protected:
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent *ev) override;
    void keyReleaseEvent(QKeyEvent *ev) override;
    void mousePressEvent( QMouseEvent* event ) override;
    void mouseReleaseEvent( QMouseEvent* event ) override;

    void resizeEvent(QResizeEvent *event) override;
    void exposeEvent(QExposeEvent *event) override;

private:
    void renderOneRect( const QRect* rect, Board* board, const PieceSet* moves, const PieceSet* tiles,
      const PieceSet* deltas, const PieceSet* shots, QPainter* painter );
    void renderRotatedPixmap(const QPixmap* pixmap, int x, int y, int angle, QPainter* painter );
    void renderMove( int x, int y, int angle = 0 );
    void renderPiece(PieceType type, int x, int y, int angle, Piece* source, QPainter* painter );
    void renderListIn(PieceSet::iterator iterator, PieceSet::iterator end, const QRect* dirty, QPainter* painter );
    void drawShotRight( int x, int y, int angle, QPainter* painter );
    void drawShotEnd( int x, int y, int angle, Piece* piece, QPainter* painter );
    QPen mPen;
    Tank* mTank;
    Shot* mShot;

    QBackingStore *mBackingStore;
    QRegion mDirtyRegion;
    QRegion mRenderRegion;

    Game* mGame;

    int mActiveMoveDirection;
};

#endif // BOARDWINDOW_H

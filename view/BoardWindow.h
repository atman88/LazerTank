#ifndef BOARDWINDOW_H
#define BOARDWINDOW_H

class BoardWindow;

#include <QtGui>
#include <QMouseEvent>
#include "tank.h"
#include "shot.h"
#include "controller/Game.h"
#include "controller/pathfinder.h"

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
    void renderRotatedPixmap(const QPixmap* pixmap, int x, int y, int angle, QPainter &painter );
    void renderMove( int x, int y, int angle = 0 );
    void renderPiece(PieceType type, int x, int y, int angle, QPainter &painter );
    void renderListAt(PieceSet::iterator *iterator, PieceSet::iterator end, Piece& pos, QPainter& painter );
    void renderListIn(PieceSet::iterator *iterator, PieceSet::iterator end, QRect& dirty, QPainter& painter );

    Tank* mTank;
    Shot* mShot;
    PathFinder mPathFinder;

    QBackingStore *mBackingStore;
    QRegion *mDirtyRegion;

    Game* mGame;

    int mActiveMoveDirection;
};

#endif // BOARDWINDOW_H

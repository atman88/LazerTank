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
    void renderPieceLater( const Piece& piece );
    void erasePiece( const Piece& );

protected:
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent *ev) override;
    void keyReleaseEvent(QKeyEvent *ev) override;

    void resizeEvent(QResizeEvent *event) override;
    void exposeEvent(QExposeEvent *event) override;

private:
    void renderMove( int x, int y, int angle = 0 );
    void renderPiece(const Piece& , QPainter *painter);
    void renderListAt(QPainter* painter, PieceList::iterator *iterator, PieceList::iterator end, int encodedPos );

    Tank* mTank;

    QBackingStore *mBackingStore;
    QRegion *mDirtyRegion;

    Game* mGame;
    QPixmap mStonePixmap;
    QPixmap mDirtPixmap;
    QPixmap mTilePixmap;
    QPixmap mMoveIndicatorPixmap;
    int mActiveMoveDirection;
};

#endif // BOARDWINDOW_H

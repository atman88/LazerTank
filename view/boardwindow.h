#ifndef BOARDWINDOW_H
#define BOARDWINDOW_H

class BoardWindow;

#include <QtGui>
#include <QMouseEvent>
#include "shotview.h"
#include "controller/game.h"

/**
 * @brief The main window
 */
class BoardWindow : public QWindow
{
    Q_OBJECT
    Q_PROPERTY(GameHandle game READ getGame WRITE setGame)

public:
    explicit BoardWindow(QWindow *parent = 0);
    ~BoardWindow()
    {
        delete mBackingStore;
    }

    GameHandle getGame() const;
    virtual void render(QRegion *region);
    void onTankKilled();

signals:
    void setSpeed( int speed );

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
    void renderOneRect( const QRect* rect, Board* board, const PieceMultiSet* moves, const PieceSet* tiles,
      const PieceSet* deltas, QPainter* painter );
    void renderMove( int x, int y, int angle = 0 );
    void renderListIn(PieceSet::iterator iterator, PieceSet::iterator end, const QRect* dirty, QPainter* painter );

    QBackingStore *mBackingStore;
    QPen mPen;

    QRegion mDirtyRegion;
    QRegion mRenderRegion;

    Game* mGame;
};

#endif // BOARDWINDOW_H

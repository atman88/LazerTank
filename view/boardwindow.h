#ifndef BOARDWINDOW_H
#define BOARDWINDOW_H

#include <QMouseEvent>
#include <QtGui>
#include <QMenu>

class Board;
class BoardWindow;
class Game;

#include "util/gameutils.h"
#include "shotview.h"

/**
 * @brief The main window
 */
class BoardWindow : public QWindow
{
    Q_OBJECT

public:
    explicit BoardWindow(QWindow *parent = 0);
    ~BoardWindow()
    {
        delete mBackingStore;
    }

    virtual void render(QRegion *region);
    void showMenu( QPoint* globalPos = 0 );
    void onTankKilled();

    QMenu& getMenu();

signals:
    void setSpeed( int speed );

public slots:
    void init( const GameHandle game );
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
    QMenu mMenu;
    QAction* mReloadAction;
    QMenu mLevelsMenu;

    QRegion mDirtyRegion;
    QRegion mRenderRegion;

    Game* mGame;
};

#endif // BOARDWINDOW_H

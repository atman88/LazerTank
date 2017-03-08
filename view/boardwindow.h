#ifndef BOARDWINDOW_H
#define BOARDWINDOW_H

#include <memory>
#include <QMouseEvent>
#include <QtGui>
#include <QMenu>

class Board;
class Game;

#include "controller/pathsearchaction.h"
#include "model/piece.h"

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
    void init( Game* game );

    /**
     * @brief pop up the menu
     * @param globalPos - optional position argument intended for mouse-related events
     */
    void showMenu( QPoint* globalPos = 0, int col = -1, int row = -1 );

    /**
     * @brief Invoke the window's UI for when the tank is destroyed
     */
    void onTankKilled();

    /**
     * @brief Access the window's popup menu
     */
    QMenu& getMenu();

signals:
    /**
     * @brief Generated by the window UI speed change logic
     * @param speed The desired speed to set
     */
    void setSpeed( int speed );

public slots:
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
    void render( const QRect* rect, Board* board, const PieceMultiSet* moves, const PieceSet* tiles,
      const PieceSet* deltas, QPainter* painter );
    void renderMove( int x, int y, int angle = 0 );
    void renderListIn(PieceSet::iterator iterator, PieceSet::iterator end, const QRect* dirty, QPainter* painter );

    QBackingStore *mBackingStore;
    QPen mPen;
    QMenu mMenu;
    QAction mSpeedAction;
    QAction mReloadAction;
    QAction mUndoMoveAction;
    QAction mClearMovesAction;
    // using shared pointers for these ui objects so they can be safely shared with the controller:
    std::shared_ptr<PathSearchAction> mCaptureAction;
    std::shared_ptr<PathSearchAction> mPathToAction;
    QMenu mLevelsMenu;

    QRegion mDirtyRegion;
    QRegion mRenderRegion;

    Game* mGame;
};

#endif // BOARDWINDOW_H
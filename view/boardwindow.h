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
    /**
     * @brief mark a rectangular area as dirty
     * @param rect The rectangular area
     */
    void renderLater(const QRect &rect);

    /**
     * @brief trigger a repaint without delay
     */
    void renderNow();

    /**
     * @brief mark a given square as dirty
     * @param col The square's column
     * @param row The square's row
     */
    void renderSquareLater( int col, int row );

    /**
     * @brief React to the game board being loaded
     */
    void onBoardLoaded();

signals:
    /**
     * @brief Notifies that the user has moved the focus
     * @param what MOVE if the focus is on the future moves or TANK if the focus is on the present
     */
    void focusChanged( PieceType what );

protected:
    /**
     * @brief Window event handlers
     */
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent *ev) override;
    void keyReleaseEvent(QKeyEvent *ev) override;
    void mousePressEvent( QMouseEvent* event ) override;
    void mouseReleaseEvent( QMouseEvent* event ) override;
    void resizeEvent(QResizeEvent *event) override;
    void exposeEvent(QExposeEvent *event) override;

private:
    /**
     * @brief Repaint a rectangular area
     * @param rect The area to repaint
     * @param painter The painter associated with this render operation
     */
    void render( const QRect* rect, QPainter* painter );

    /**
     * @brief Helper method to render the given set within the given rectangular area
     * @param iterator The starting position within the set
     * @param end The end of set position
     * @param dirty The rectangular area being rendered
     * @param painter The painter associated with this render operation
     */
    void renderListIn( PieceSet::iterator iterator, PieceSet::iterator end, const QRect* dirty, QPainter* painter );

    /**
     * @brief Move the focus between focal points of interest
     * @param what Either MOVE to focus on the last move (future perspective) or TANK to focus on the first move
     * (present perspective)
     */
    void moveFocus( PieceType what );

    QBackingStore *mBackingStore;
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

    PieceType mFocusType;

    Game* mGame;
};

#endif // BOARDWINDOW_H

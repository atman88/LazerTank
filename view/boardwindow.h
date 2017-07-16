#ifndef BOARDWINDOW_H
#define BOARDWINDOW_H

#include <Qt>
#include <QtGui>
#include <QMenu>
#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QMouseEvent)
QT_FORWARD_DECLARE_CLASS(QTextBrowser)

class Board;
class GameRegistry;
class Game;
class PieceListManager;
class PathSearchAction;
class ReplayText;

#include "boardrenderer.h"
#include "controller/dragactivity.h"
#include "model/piece.h"

#if (QT_VERSION >= QT_VERSION_CHECK(5, 7, 0))
#define ACTION QAction
#define TO_QACTION(a) a
#else
class ActionProxy
{
public:
    ActionProxy() : mAction(0)
    {
    }
    ~ActionProxy()
    {
        if ( mAction ) {
            delete mAction;
        }
    }

    QAction* getAction()
    {
        if ( !mAction ) {
            mAction = new QAction(0);
        }
        return mAction;
    }

private:
    QAction* mAction;
};

#define ACTION ActionProxy
#define TO_QACTION(a) (*(a).getAction())
#endif

/**
 * @brief The main window
 */
class BoardWindow : public QWindow
{
    Q_OBJECT

public:
    explicit BoardWindow(QWindow *parent = 0);
    ~BoardWindow();
    void init( GameRegistry* registry );

    /**
     * @brief pop up the menu
     * @param globalPos - optional position argument intended for mouse-related events
     */
    void showMenu( QPoint* globalPos = 0, ModelPoint p = ModelPoint() );

    /**
     * @brief Access the window's popup menu
     */
    QMenu& getMenu();

    /**
     * @brief Query if painting has occured
     * @return true this window has painted one or more times
     */
    bool isPaintable() const;

    /**
     * @brief Confirms whether replay is active
     * @return 0 if not active, 1 if active or -1 if set inactive as a result of this call
     */
    int checkForReplay();

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
     * @brief mark a given square as dirty
     * @param col The square's column
     * @param row The square's row
     */
    void renderSquareLater( int col, int row );

private slots:
    /**
     * @brief trigger a repaint without delay
     */
    void renderNow();

    /**
     * @brief React to the game board being loaded
     */
    void onBoardLoaded();

    /**
     * @brief Display the game help
     */
    void showHelp();

    /**
     * @brief Select a level
     */
    void chooseLevel();

    /**
     * @brief Load a level
     * @param number The level number
     */
    void loadLevel( int number );

    /**
     * @brief Listens to path events for the purpose of starting drag activities on them as appropriate
     */
    void onPathFound( PieceListManager* path, PathSearchAction* action );

    /**
     * @brief Listens to the state of its drag activity for display purposes
     */
    void onDragStateChange();

signals:
    /**
     * @brief Notifies that the user has moved the focus
     * @param what MOVE if the focus is on the future moves or TANK if the focus is on the present
     */
    void focusChanged( PieceType what );

    /**
     * @brief Emitted the first time the window has a paintable surface
     */
    void paintable();

protected:
    /**
     * @brief Window event handlers
     */
    bool event(QEvent *event) override;
    void keyPressEvent(QKeyEvent *ev) override;
    void keyReleaseEvent(QKeyEvent *ev) override;
    void mousePressEvent( QMouseEvent* event ) override;
    void mouseReleaseEvent( QMouseEvent* event ) override;
    void mouseMoveEvent( QMouseEvent* event ) override;
    void showEvent(QShowEvent*) override;
    void resizeEvent(QResizeEvent *event) override;
    void exposeEvent(QExposeEvent *event) override;

private:
    /**
     * @brief Size the window to fit the given number of columns and rows
     * @param cols The number of columns
     * @param rows The number of rows
     */
    void setSize( int cols, int rows );

    /**
     * @brief Event handler helper to resize the backing store. Rendering is managed by the caller.
     * @param size the new size
     * @return true if changed
     */
    bool resizeInternal( const QSize& size );

    /**
     * @brief Repaint a rectangular area
     * @param rect The area to repaint
     * @param registry The valid game registry
     * @param painter The painter associated with this render operation
     */
    void render( const QRect* rect, GameRegistry* registry, QPainter* painter );

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
    BoardRenderer mRenderer;
    QMenu mMenu;
    ACTION mSpeedAction;
    ACTION mReloadAction;
    ACTION mUndoMoveAction;
    ACTION mClearMovesAction;
    ACTION mReplayAction;

    QRegion mDirtyRegion;
    QRegion mRenderRegion;
    bool mRenderedOnce;

    PieceType mFocus;

    bool mGameInitialized;
    QTextBrowser* mHelpWidget;

    ReplayText* mReplayText;
    DragActivity mDragActivity;
    bool mMouseLeftDown;

#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
    void requestUpdate();
    bool mUpdatePending;
#endif
};

#endif // BOARDWINDOW_H

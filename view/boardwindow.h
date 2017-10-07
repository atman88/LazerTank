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
class ReplayText;

#include "boardrenderer.h"
#include "tiledragmarker.h"
#include "controller/movecontroller.h"
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

    void connectTo( const PieceManager& manager ) const;

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
    void renderLater( const QRect &rect );

    /**
     * @brief mark a given square as dirty
     */
    void renderSquareLater( ModelPoint point );

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
     * @brief Listens for level changes to update the status bar
     * @param index The level that was updated
     */
    void onLevelUpdated( const QModelIndex& index );

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
     * @brief Update the cursor to depict the given drag state
     */
    void setCursorDragState( DragState state );

    /**
     * @brief Recieves notification that the recording count changed
     */
    void onRecordedCountChanged();

signals:
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
    void renderBoard( const QRect* rect, GameRegistry* registry, QPainter* painter );

    void renderStatus( QRect& statusRect, GameRegistry* registry, QPainter* painter );

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

    bool mGameInitialized;
    QTextBrowser* mHelpWidget;

    ReplayText* mReplayText;

    QCursor* mForbiddenCursor;

    TileDragMarker mDragMarker;

    bool mStatusDirty;

#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
    void requestUpdate();
    bool mUpdatePending;
#endif
};

#endif // BOARDWINDOW_H

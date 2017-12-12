#ifndef BOARDWINDOW_H
#define BOARDWINDOW_H

#include <Qt>
#include <QtGui>
#include <QMenu>
#include <QtGlobal>
#include <QMainWindow>

QT_FORWARD_DECLARE_CLASS(QMouseEvent)
QT_FORWARD_DECLARE_CLASS(QTextBrowser)
QT_FORWARD_DECLARE_CLASS(QLabel)

class Board;
class GameRegistry;
class Game;
class ReplayText;

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

class BoardWidget : public QWidget
{
    Q_OBJECT
    const int TILE_SIZE = 24;

public:
    BoardWidget( QWidget* parent = 0 );
    ~BoardWidget();
    void init( GameRegistry* registry );

    void paintEvent( QPaintEvent* e ) override;
    QSize sizeHint() const override;

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
    void onBoardLoaded();

protected:
    void mousePressEvent( QMouseEvent* event ) override;
    void mouseReleaseEvent( QMouseEvent* event ) override;
    void mouseMoveEvent( QMouseEvent* event ) override;
    bool event( QEvent* event ) override;

private:
    /**
     * @brief Update the cursor to depict the given drag state
     */
    void setCursorDragState( DragState state );

    TileDragMarker mDragMarker;
    QCursor* mForbiddenCursor;
    QAction mWhatsThisAction;
};

/**
 * @brief The main window
 */
class BoardWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit BoardWindow(QWidget *parent = 0);
    ~BoardWindow();
    void init( GameRegistry* registry );

    /**
     * @brief pop up the menu
     * @param globalPos Optional position argument
     * @param widgetActions Any additional menu items
     * @param widgetSearchActions Any widgetActions that are search terms
     * @return The action activated or 0
     */
    QAction* showMenu( QPoint* globalPos = 0, const QList<QAction*> widgetActions = QList<QAction*>(), const QList<PathSearchAction*> widgetSearchActions = QList<PathSearchAction*>() );

    /**
     * @brief Access the window's popup menu
     */
    QMenu& getMenu();

    void connectTo( const PieceManager& manager ) const;

signals:
    /**
     * @brief Generated by the window UI speed change logic
     * @param speed The desired speed to set
     */
    void setSpeed( int speed );

private slots:
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
     * @brief Recieves notification that the recording count changed
     */
    void onRecordedCountChanged();

protected:
    /**
     * @brief Window event handlers
     */
    void keyPressEvent(QKeyEvent *ev) override;
    void keyReleaseEvent(QKeyEvent *ev) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    /**
     * @brief Event handler helper to resize the backing store. Rendering is managed by the caller.
     * @param size the new size
     * @return true if changed
     */
    bool resizeInternal(const QSize&);

    QMenu mMenu;
    ACTION mSpeedAction;
    ACTION mReloadAction;
    ACTION mUndoMoveAction;
    ACTION mClearMovesAction;
    ACTION mReplayAction;

    QLabel* mMoveCounter;
    QLabel* mSavedMoveCount;
    QLabel* mCompletedIndicator;

    QRegion mDirtyRegion;
    QRegion mRenderRegion;

    bool mGameInitialized;
    QTextBrowser* mHelpWidget;

    ReplayText* mReplayText;

#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
    void requestUpdate();
    bool mUpdatePending;
#endif
};

#endif // BOARDWINDOW_H

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
#include "whatsthisaware.h"
#include "controller/movecontroller.h"
#include "model/piece.h"


class BoardWidget : public QWidget
{
    Q_OBJECT
    const int TILE_SIZE = 24;

public:
    BoardWidget( QWidget* parent = nullptr );
    ~BoardWidget() override;
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
    explicit BoardWindow(QWidget *parent = nullptr);
    ~BoardWindow() override;
    void init( GameRegistry* registry );

    /**
     * @brief pop up the menu
     * @param globalPos Optional position argument
     * @param widgetActions Any additional menu items
     * @param widgetSearchActions Any widgetActions that are search terms
     * @return The action activated or 0
     */
    QAction* showMenu(QPoint* globalPos = nullptr, QList<QAction*> widgetActions = QList<QAction*>(), const QList<PathSearchAction *>& widgetSearchActions = QList<PathSearchAction*>() );

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

    /**
     * @brief Notifies that a backdoor key sequence has been entered.
     * Back doors are generated by holding down the shift key and typing 2-4 alphabetic characters
     * @param code The alphabetic sequence as an integer value, where the first character is the upper
     * most used byte and the last is the least significant byte.
     */
    void backdoor( int code );

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

private:
    QMenu mMenu;
    QAction mSpeedAction;
    QAction mReloadAction;
    QAction mUndoMoveAction;
    QAction mClearMovesAction;
    QAction mReplayAction;

    QLabel* mMoveCounter;
    QLabel* mSavedMoveCount;
    QLabel* mCompletedIndicator;

    QRegion mDirtyRegion;
    QRegion mRenderRegion;

    bool mGameInitialized;
    QTextBrowser* mHelpWidget;

    ReplayText* mReplayText;

    int mBackdoorCode;

#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
    void requestUpdate();
    bool mUpdatePending;
#endif
};

#endif // BOARDWINDOW_H

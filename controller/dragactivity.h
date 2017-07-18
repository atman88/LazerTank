#ifndef DRAGACTIVITY_H
#define DRAGACTIVITY_H

#include <QObject>

class GameRegistry;
class PieceListManager;
class PathSearchAction;

#include "model/modelpoint.h"
#include "model/piece.h"

typedef enum {
    Inactive,
    Searching,
    Selecting,
    Forbidden
} DragState;

class DragActivity : public QObject
{
    Q_OBJECT
public:
    explicit DragActivity( QObject* parent = 0 );
    void init( GameRegistry* registry );

    /**
     * @brief Retrieve the currsor associated with the current state
     * @return The current cursor or 0 if not started
     */
    DragState getState() const;

    /**
     * @brief Retrieve the current drag point
     * @return The current end point of the drag, or a null point if that drag is not started
     */
    ModelPoint getFocusPoint() const;

    /**
     * @brief Handles a drag to the given point
     * @param p The point dragged to
     */
    void onDragTo( ModelPoint p );

    /**
     * @brief Begin a drag activity
     * @param startPoint The initial point of the drag
     * @param focus Either MOVE to append any outstanding future moves or TANK to replace future moves
     */
    void start( ModelPoint startPoint, PieceType focus = MOVE );

signals:
    /**
     * @brief Indicates that a change of state occured
     */
    void stateChanged( DragState state );

public slots:
    /**
     * @brief Completes the outstanding drag activity if any
     */
    void stop();

private slots:
    /**
     * @brief Listens to path events for starting drag activities when self-initiated
     */
    void onPathFound( PieceListManager* path, PathSearchAction* action );

private:
    void setState( DragState state );

    ModelPoint mFocusPoint;
    DragState mState;
    bool mChanged;
};

#endif // DRAGACTIVITY_H

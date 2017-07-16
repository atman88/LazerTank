#ifndef DRAGACTIVITY_H
#define DRAGACTIVITY_H

#include <QObject>

#include "modelpoint.h"

typedef enum {
    Inactive,
    Selecting,
    Forbidden
} DragState;

class DragActivity : public QObject
{
    Q_OBJECT
public:
    explicit DragActivity( QObject* parent = 0 );

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

signals:
    /**
     * @brief Indicates that a change of state occured
     */
    void stateChanged();

public slots:
    /**
     * @brief Begin a drag activity
     * @param startPoint The initial point of the drag
     */
    void start( ModelPoint startPoint );

    /**
     * @brief Completes the outstanding drag activity if any
     */
    void stop();

private:
    void setState( DragState state );

    ModelPoint mFocusPoint;
    DragState mState;
    bool mChanged;
};

#endif // DRAGACTIVITY_H

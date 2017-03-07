#ifndef PUSH_H
#define PUSH_H

#include <QObject>
#include <QPropertyAnimation>
#include <QRect>
#include <QPainter>

class Game;

#include "controller/speedcontroller.h"
#include "model/piece.h"

/**
 * @brief implements pushing a piece on the board
 */
class Push : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant pieceX READ getX WRITE setX)
    Q_PROPERTY(QVariant pieceY READ getY WRITE setY)

public:
    explicit Push(QObject *parent = 0);
    void init( Game* game );
    void render( const QRect* rect, QPainter* painter );

    /**
     * @brief Get the type of piece being pushed
     * @return The type of piece or NONE if it is not pushing anything
     */
    PieceType getType();

    /**
     * @brief Get the upper left x window coordinate of the piece
     * @return Upper left x coordinate
     */
    QVariant getX();

    /**
     * @brief Get the upper left y window coordinate of the piece
     * @return Upper left y coordinate
     */
    QVariant getY();

    /**
     * @brief Get the current angle of orientation of the piece
     * @return The piece's orientation. One of 0, 90, 180 or 270.
     */
    int getPieceAngle();

    /**
     * @brief Get the rectangular window region that this piece currently occupies
     * @return The rectangular window region occupied
     */
    QRect* getBounds();

    /**
     * @brief start a push sequence between two squares
     * @param what The type of piece to show being pushed
     * @param fromX starting upper left x window coordinate
     * @param fromY starting upper left y window coordinate
     * @param toX destination upper left x window coordinate
     * @param toY destination upper left Y window coordinate
     */
    void start(Piece& what, int fromX, int fromY, int toX, int toY);

signals:
    void rectDirty( const QRect& );

    /**
     * @brief Notifies when the push is starting/stopping
     * @param newState State being entered
     * @param oldState State being exitted
     */
    void stateChanged(QAbstractAnimation::State newState, QAbstractAnimation::State oldState);

public slots:
    /**
     * @brief setX/SetY/onStopped - These slots are solely for use by internal animations
     * @param x
     */
    void setX( const QVariant& x );
    void setY( const QVariant& y );
    void onStopped();

private:
    MoveSpeedControlledAnimation mHorizontalAnimation;
    MoveSpeedControlledAnimation mVerticalAnimation;
    PieceType mType;
    int mPieceAngle;
    QRect mBoundingRect;
    QRect mRenderedBoundingRect;
};

#endif // PUSH_H

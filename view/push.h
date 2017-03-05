#ifndef PUSH_H
#define PUSH_H

#include <QObject>
#include <QPropertyAnimation>
#include <QRect>
#include <QPainter>
#include "controller/speedcontroller.h"
#include "model/piece.h"
#include "model/board.h"

class Game;

class Push : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant pieceX READ getX WRITE setX)
    Q_PROPERTY(QVariant pieceY READ getY WRITE setY)

public:
    explicit Push(QObject *parent = 0);
    void init( Game* game );
    PieceType getType();
    QVariant getX();
    QVariant getY();
    int getPieceAngle();
    QRect* getBounds();
    void start(Piece& what, int fromX, int fromY, int toX, int toY);
    int getEndX();
    int getEndY();
    void render( const QRect* rect, QPainter* painter );

signals:
    void rectDirty( const QRect& );
    void stateChanged(QAbstractAnimation::State newState, QAbstractAnimation::State oldState);

public slots:
    void setX( const QVariant& x );
    void setY( const QVariant& y );
    void onStopped();

private:
    Board* getBoard();

    MoveSpeedControlledAnimation mHorizontalAnimation;
    MoveSpeedControlledAnimation mVerticalAnimation;
    PieceType mType;
    int mPieceAngle;
    QRect mBoundingRect;
    QRect mRenderedBoundingRect;
};

#endif // PUSH_H

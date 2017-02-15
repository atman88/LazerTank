#ifndef PUSH_H
#define PUSH_H

#include <QObject>
#include <QPropertyAnimation>
#include <QRect>

#include "model/piece.h"
#include "model/board.h"

class Push : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant pieceX READ getX WRITE setX)
    Q_PROPERTY(QVariant pieceY READ getY WRITE setY)

public:
    explicit Push(QObject *parent = 0);
    PieceType getType();
    QVariant getX();
    QVariant getY();
    void start(PieceType what, int fromX, int fromY, int toX, int toY);

signals:
    void pieceMoved( const QRect& );

public slots:
    void setX( const QVariant& x );
    void setY( const QVariant& y );
    void onStopped();

private:
    Board* getBoard();

    QPropertyAnimation* mHorizontalAnimation;
    QPropertyAnimation* mVerticalAnimation;
    PieceType mType;
    QRect mBoundingRect;
};

#endif // PUSH_H

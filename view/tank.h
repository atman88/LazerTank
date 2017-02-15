#ifndef TANK_H
#define TANK_H

#include <QObject>
#include <QPainter>
#include <QPropertyAnimation>

#include "model/piece.h"

class Tank : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant rotation READ getRotation WRITE setRotation)
    Q_PROPERTY(QVariant x READ getX WRITE setX)
    Q_PROPERTY(QVariant y READ getY WRITE setY)

public:
    explicit Tank(QObject *parent = Q_NULLPTR );
    ~Tank() {
        if ( mRotateAnimation ) {
            delete mRotateAnimation;
        }
    }

    PieceList& getMoves();
    void paint( QPainter* painter );
    void move( int direction );
    bool isMoving();
    QVariant getRotation();
    QVariant getX();
    QVariant getY();
    const QRect& getRect();

signals:
    void changed( const QRect& rect );
    void moved( int boardX, int boardY );
    void pathAdded( Piece& piece );

public slots:
    void reset( int x, int y );
    void setRotation(const QVariant &angle );
    void setX(const QVariant &x );
    void setY(const QVariant &y );
    void animationFinished();

private:
    bool followPath();
    bool isStopped();
    void animateMove( int from, int to, QPropertyAnimation *animation );

    QPixmap mPixmap;
    QRect mBoundingRect;
    QVariant mRotation;
    QPropertyAnimation* mRotateAnimation;
    QPropertyAnimation* mHorizontalAnimation;
    QPropertyAnimation* mVerticalAnimation;

    PieceList mMoves;
};

#endif // TANK_H

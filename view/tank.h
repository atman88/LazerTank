#ifndef TANK_H
#define TANK_H

#include <QObject>
#include <QPainter>
#include <QPropertyAnimation>

#include "model/piece.h"

class Game;

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

    void init( Game* game );
    PieceList& getMoves();
    void paint( QPainter* painter );
    void move( int direction );
    bool isMoving();
    void eraseLastMove();
    QVariant getRotation();
    QVariant getX();
    QVariant getY();
    const QRect& getRect();

signals:
    void changed( const QRect& rect );
    void moved( int boardX, int boardY );
    void pieceDirty( Piece& piece );
    void movingInto( int x, int y, int curRotation );

public slots:
    void reset( int x, int y );
    void setRotation(const QVariant &angle );
    void setX(const QVariant &x );
    void setY(const QVariant &y );
    void onAnimationsFinished();

private:
    void followPath();
    void animateMove( int from, int to, QPropertyAnimation *animation );
    Game* getGame();

    QPixmap mPixmap;
    QRect mBoundingRect;
    QRect mPreviousPaintRect;
    QVariant mRotation;
    QPropertyAnimation* mRotateAnimation;
    QPropertyAnimation* mHorizontalAnimation;
    QPropertyAnimation* mVerticalAnimation;

    PieceList mMoves;
};

#endif // TANK_H

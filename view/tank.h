#ifndef TANK_H
#define TANK_H

#include <QObject>
#include <QPainter>
#include <QPropertyAnimation>

#include "view/shooter.h"
#include "model/piece.h"

class Game;

class Tank : public Shooter
{
    Q_OBJECT

public:
    Tank(QObject *parent = 0);
    virtual ~Tank() {}

    void init( Game* game );
    PieceList& getMoves();
    void paint( QPainter* painter );
    void move( int direction );
    void eraseLastMove();

signals:
    void changed( const QRect& rect );
    void moved( int boardX, int boardY );
    void pieceDirty( Piece& piece );
    void movingInto( int x, int y, int curRotation );

public slots:
    void reset( int x, int y );
    void onAnimationsFinished();
    void setX(const QVariant &x ) override;
    void setY(const QVariant &y ) override;
    void setRotation( const QVariant& angle ) override;

private:
    void followPath();
    void animateMove( int from, int to, QPropertyAnimation *animation );
    Game* getGame();

    QPixmap mPixmap;
    QRect mPreviousPaintRect;
    QPropertyAnimation* mRotateAnimation;
    QPropertyAnimation* mHorizontalAnimation;
    QPropertyAnimation* mVerticalAnimation;

    PieceList mMoves;
};

#endif // TANK_H

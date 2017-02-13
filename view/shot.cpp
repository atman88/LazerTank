#include "shot.h"
#include "Game.h"

Shot::Shot(QObject *parent) : QObject(parent)
{
    mAnimation = new QPropertyAnimation(this,"sequence");
    QObject::connect(mAnimation, &QVariantAnimation::finished,this,&Shot::animationFinished);
    mAnimation->setStartValue(0);
    mAnimation->setEndValue(BOARD_MAX_WIDTH * BOARD_MAX_HEIGHT); // use a reasonable watermark value
    mAnimation->setDuration(60*1000);

    setProperty("path", QVariant::fromValue(mPath));
}

QVariant Shot::getSequence()
{
    return mSequence;
}

void Shot::setSequence( const QVariant &sequence )
{
    int seq = mSequence.toInt();
    if ( seq < sequence.toInt() ) {
        QObject* p = parent();
        QVariant v = p->property("GameHandle");
        Game* game = v.value<GameHandle>().game;

        int direction, x, y;
        if ( !mPath.size() ) {
            direction = mDirection;
            x = game->getTankX();
            y = game->getTankY();
        } else {
            Piece& p = mPath.back();
            direction = p.getAngle();
            x = p.getX();
            y = p.getY();
        }

        do {
            if ( !game->canShootFrom( direction, &x, &y ) ) {
                mAnimation->stop();
                break;
            }
            mPath.push_back( Piece(SHOT_STRAIGHT,x,y,direction) );
            setProperty("path", QVariant::fromValue(mPath));
            emit pathAdded( mPath.back() );
        } while( ++seq < sequence.toInt() );
        mSequence = sequence;
    }
}

void Shot::stop()
{
    mAnimation->stop();

    for( auto it = mPath.begin(); it != mPath.end(); ++it ) {
        emit pathRemoved( *it );
    }
    mPath.clear();
    setProperty("path", QVariant::fromValue(mPath));
}

void Shot::animationFinished()
{
    stop();
}

void Shot::fire( int direction, int x, int y )
{
    stop();

    QObject* p = parent();
    QVariant v = p->property("GameHandle");
    Game* game = v.value<GameHandle>().game;
    if ( game->canShootFrom( direction, &x, &y ) ) {
        mSequence = QVariant(-1);
        mDirection = direction;
        mAnimation->start();

        emit pathAdded(mPath.back());
    }
}

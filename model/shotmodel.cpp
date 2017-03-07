#include "shotmodel.h"
#include "controller/game.h"

ShotModel::ShotModel(QObject *parent) : ShotView(parent), mLeadingCol(-1), mLeadingRow(-1), mDistance(0), mShedding(false),
  mKillSequence(0)
{
    mAnimation.setTargetObject(this);
    mAnimation.setPropertyName("sequence");
    mAnimation.setStartValue(0);
    mAnimation.setEndValue(BOARD_MAX_WIDTH * BOARD_MAX_HEIGHT); // use a reasonable watermark value
    mAnimation.setDuration(60/8 * BOARD_MAX_WIDTH * BOARD_MAX_HEIGHT);
}

void ShotModel::reset()
{
    mLeadingCol = mLeadingRow = -1;
    mSequence = QVariant(-1);
    mDistance = 0;
    mShedding = false;
    mKillSequence = 0;
    ShotView::reset();
}

void ShotModel::init( AnimationStateAggregator* aggregate )
{
    QObject::connect( &mAnimation, &QPropertyAnimation::stateChanged, aggregate, &AnimationStateAggregator::onStateChanged );
}

QVariant ShotModel::getSequence()
{
    return mSequence;
}

void ShotModel::fire( Shooter* shooter )
{
    int direction = shooter->getRotation().toInt();
    if ( !(direction % 90) ) {
        mAnimation.stop();
        reset();
        mLeadingDirection = direction;
        mLeadingCol = shooter->getX().toInt()/24;
        mLeadingRow = shooter->getY().toInt()/24;
        commenceFire( shooter );
        mAnimation.start();
    }
}

void ShotModel::setSequence( const QVariant &sequence )
{
    mSequence = sequence;

    Game* game = getGame();
    if ( !game ) {
        return;
    }

    if ( mKillSequence ) {
        switch( mKillSequence++ ) {
        case 1:
            killTheTank();
            break;
        case 3:
            mAnimation.pause();
            emit tankKilled();
            break;
        default:
            break;
        }
        return;
    }

    if ( !hasTerminationPoint() ) {
        int startDirection = mLeadingDirection;
        int endOffset;
        if ( game->advanceShot( &mLeadingDirection, &mLeadingCol, &mLeadingRow, &endOffset, this ) ) {
            grow( mLeadingCol, mLeadingRow, startDirection, mLeadingDirection );
        } else {
            addTermination( startDirection, endOffset );
            mShedding = true;
        }
    }

    bool isTravelling = true;
    if ( ++mDistance > 5 && mShedding && !mKillSequence ) {
        isTravelling = shedTail();
    }

    if ( !isTravelling && !mKillSequence ) {
        mAnimation.stop();
    }
}

void ShotModel::startShedding()
{
    mShedding = true;
}

void ShotModel::setIsKill()
{
    mKillSequence = 1;
}

Game* ShotModel::getGame()
{
    // find the game from the object hierarchy:
    QObject* p = parent();
    QVariant v;
    while( p && !(v = p->property("GameHandle")).isValid() ) {
        p = p->parent();
    }
    return v.value<GameHandle>().game;
}

int ShotModel::getLeadingRow() const
{
    return mLeadingRow;
}

void ShotModel::setLeadingRow(int leadingRow)
{
    mLeadingRow = leadingRow;
}

void ShotModel::setLeadingCol(int leadingCol)
{
    mLeadingCol = leadingCol;
}

int ShotModel::getLeadingCol() const
{
    return mLeadingCol;
}

#include "shotmodel.h"
#include "controller/game.h"
#include "util/gameutils.h"

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

int ShotModel::getDistance() const
{
    return mDistance;
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
    int direction = shooter->getViewRotation().toInt() % 360;
    if ( !(direction % 90) ) {
        mAnimation.stop();
        reset();
        mLeadingDirection = direction;
        mLeadingCol = shooter->getViewX().toInt()/24;
        mLeadingRow = shooter->getViewY().toInt()/24;
        commenceFire( shooter );
        mAnimation.start();
    }
}

void ShotModel::setSequence( const QVariant &sequence )
{
    mSequence = sequence;

    Game* game = getGame(this);
    if ( !game ) {
        return;
    }

    if ( mKillSequence ) {
        switch( mKillSequence++ ) {
        case 1:
            killTheTank();
            break;
        case 3:
            mAnimation.pause(); // freeze the shot to show the kill
            emit tankKilled();
            break;
        default:
            break;
        }
        return;
    }

    if ( !hasTerminationPoint() ) {
        if ( getAdjacentPosition( mLeadingDirection, &mLeadingCol, &mLeadingRow ) ) {
            QPoint hitPoint( modelToViewCenterSquare(mLeadingCol,mLeadingRow) );
            int entryDirection = mLeadingDirection;
            if ( game->canShootThru( mLeadingCol, mLeadingRow, &mLeadingDirection, getShooter(), &hitPoint ) ) {
                grow( modelToViewCenterSquare(mLeadingCol,mLeadingRow), entryDirection );
            } else {
                addTermination( entryDirection, hitPoint );
                mShedding = true;
            }
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

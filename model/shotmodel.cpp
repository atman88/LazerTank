#include "shotmodel.h"
#include "controller/game.h"
#include "controller/animationstateaggregator.h"
#include "view/shooter.h"
#include "util/gameutils.h"

ShotModel::ShotModel(QObject *parent) : ShotView(parent), mDistance(0), mShedding(false),
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
    mAnimation.stop();
    mLeadingPoint.setNull();
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

void ShotModel::init(AnimationStateAggregator& aggregate )
{
    QObject::connect( &mAnimation, &QPropertyAnimation::stateChanged, &aggregate, &AnimationStateAggregator::onStateChanged );
}

QVariant ShotModel::getSequence()
{
    return mSequence;
}

bool ShotModel::fire( Shooter* shooter )
{
    int direction = shooter->getViewRotation().toInt() % 360;
    if ( !(direction % 90) ) {
        reset();
        mLeadingDirection = direction;
        mLeadingPoint = ModelPoint( shooter->getViewX().toInt()/24, shooter->getViewY().toInt()/24 );
        commenceFire( shooter );
        mAnimation.start();
        return true;
    }
    return false;
}

void ShotModel::setSequence( const QVariant &sequence )
{
    mSequence = sequence;

    if ( GameRegistry* registry = getRegistry(this) ) {
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
            if ( getAdjacentPosition( mLeadingDirection, &mLeadingPoint ) ) {
                QPoint hitPoint = mLeadingPoint.toViewCenterSquare();
                int entryDirection = mLeadingDirection;
                if ( registry->getGame().canShootThru( mLeadingPoint, &mLeadingDirection, 0, getShooter(), &hitPoint ) ) {
                    grow( mLeadingPoint.toViewCenterSquare(), entryDirection );
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
            reset(); // done
        }
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

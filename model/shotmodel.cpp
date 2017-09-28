#include <iostream>
#include <mutex>
#include "shotmodel.h"
#include "controller/game.h"
#include "controller/animationstateaggregator.h"
#include "view/shooter.h"
#include "util/gameutils.h"

// Runnable to measure the total length of the current shot.
// Note its implementation uses a mutex to pass its result between threads.
//
class MeasureRunnable : public BasicRunnable
{
public:
    MeasureRunnable( ShotModel& model ) : mModel(model), mResult(0)
    {
    }

    void startMeasurement( ModelVector startVector )
    {
        if ( GameRegistry* registry = getRegistry(&mModel) ) {
            {   std::lock_guard<std::mutex> guard(mMutex);
                mStartVector = startVector;
                mResult = 0;
            }
            registry->getWorker().doWork(this);
        }
    }

    int getMeasurementFor( ModelVector startVector )
    {
        std::lock_guard<std::mutex> guard(mMutex);
        return startVector.equals( mStartVector ) ? mResult : 0;
    }

    void run()
    {
        if ( GameRegistry* registry = getRegistry(&mModel) ) {
            ModelVector curVector;
            {   std::lock_guard<std::mutex> guard(mMutex);
                curVector = mStartVector;
            }
            ModelVector startVector( curVector );
            Game& game = registry->getGame();

            int length = 0;
            while( getAdjacentPosition( curVector.mAngle, &curVector )
              && game.canShootThru( curVector, &curVector.mAngle )
              && !curVector.ModelPoint::equals(startVector) /*prevent infinite circular path*/ ) {
                ++length;
            }
            if ( length ) {
                std::lock_guard<std::mutex> guard(mMutex);
                if ( mStartVector.equals(startVector) ) {
                    mResult = length;
                }
            }
        }
    }

private:
    std::mutex mMutex;
    ShotModel& mModel;
    ModelVector mStartVector;
    int mResult;
};

ShotModel::ShotModel(QObject *parent) : ShotView(parent), mLeadingDirection(0), mDistance(0), mShedding(false),
  mKillSequence(0), mRunnable(new MeasureRunnable(*this))
{
    mAnimation.setTargetObject(this);
    mAnimation.setPropertyName("sequence");
    mAnimation.setStartValue(0);
    mAnimation.setEndValue(BOARD_MAX_WIDTH * BOARD_MAX_HEIGHT); // use a reasonable watermark value
    mAnimation.setDuration(60/8 * BOARD_MAX_WIDTH * BOARD_MAX_HEIGHT);
}

ShotModel::~ShotModel()
{
    delete mRunnable;
}

void ShotModel::reset()
{
    mAnimation.stop();
    mStartVector.setNull();
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
        mStartVector = ModelVector( mLeadingPoint, direction );
        commenceFire( shooter );
        mAnimation.start();
        mRunnable->startMeasurement( mStartVector );
        return true;
    }
    return false;
}

void ShotModel::setSequence( const QVariant &sequence )
{
    mSequence = sequence;

    if ( GameRegistry* registry = getRegistry(this) ) {
        Game& game = registry->getGame();

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

        // intializing anticipatedLength here such that:
        // a) it uses a minimum of 5. This delays tail shedding which in turn slows down short shots so they don't
        //    flash uncomfortably quickly
        // b) It uses the measured length of the shot if/when available, otherwise it defaults to the minimum
        //
        int anticipatedLength = std::max( getMeasurement(), 5 );

        int iteration = 0;
        do {
            if ( !hasTerminationPoint() ) {
                if ( getAdjacentPosition( mLeadingDirection, &mLeadingPoint ) ) {
                    QPoint hitPoint = mLeadingPoint.toViewCenterSquare();
                    int entryDirection = mLeadingDirection;
                    if ( game.canShootThru( mLeadingPoint, &mLeadingDirection, 0, true, getShooter(), &hitPoint ) ) {
                        grow( mLeadingPoint.toViewCenterSquare(), entryDirection );
                    } else {
                        addTermination( entryDirection, hitPoint );
                        mShedding = true;
                    }
                }
            }

            if ( mKillSequence ) {
                break;
            }

            bool isTravelling = true;
            if ( ++mDistance >= anticipatedLength && mShedding ) {
                isTravelling = shedTail();
            }

            if ( !isTravelling ) {
                reset(); // done
                break;
            }
            // repeat such that travel should complete after ~four sequence events
        } while( ++iteration <= anticipatedLength/4 );
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

int ShotModel::getMeasurement() const
{
    return mRunnable->getMeasurementFor( mStartVector );
}

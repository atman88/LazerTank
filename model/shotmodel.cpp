#include <iostream>
#include <mutex>
#include "shotmodel.h"
#include "controller/game.h"
#include "controller/animationstateaggregator.h"
#include "controller/speedcontroller.h"
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

    void startMeasurement( const ModelVector& startVector )
    {
        if ( GameRegistry* registry = getRegistry(&mModel) ) {
            {   std::lock_guard<std::mutex> guard(mMutex);
                mStartVector = startVector;
                mResult = 0;
            }
            registry->getWorker().doWork(this);
        }
    }

    int getMeasurementFor( const ModelVector& startVector )
    {
        std::lock_guard<std::mutex> guard(mMutex);
        return startVector.equals( mStartVector ) ? mResult : 0;
    }

    void run() final
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

ShotModel::ShotModel( QObject* parent ) : ShotView(parent), mLeadingDirection{0}, mDistance{0}, mShedding{false},
  mKillSequence{0}, mLastStepNo{-1}, mDuration{-1}, mRunnable(new MeasureRunnable(*this))
{
    QObject::connect( &mAnimation, &ShotAnimation::currentTimeChanged, this, &ShotModel::onTimeChanged, Qt::DirectConnection );
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
    mDistance = 0;
    mShedding = false;
    mKillSequence = 0;
    mLastStepNo = -1;
    mDuration = -1;
    ShotView::reset();
}

int ShotModel::getDistance() const
{
    return mDistance;
}

void ShotModel::init( AnimationStateAggregator& aggregate )
{
    QObject::connect( &mAnimation, &ShotAnimation::stateChanged, &aggregate, &AnimationStateAggregator::onStateChanged );
}

bool ShotModel::fire( Shooter* shooter )
{
    int direction = shooter->getViewRotation().toInt() % 360;
    if ( !(direction % 90) ) {
        reset();
        mLeadingDirection = direction;
        mLeadingPoint = ModelPoint( shooter->getViewX().toInt()/24, shooter->getViewY().toInt()/24 );
        mStartVector = ModelVector( mLeadingPoint, direction );
        if ( GameRegistry* registry = getRegistry(this) ) {
            mDuration = registry->getSpeedController().getSpeed() - 30;
        } else {
            mDuration = SpeedController::NormalSpeed - 30;
        }
        commenceFire( shooter );
        mAnimation.start();
        mRunnable->startMeasurement( mStartVector );
        return true;
    }
    return false;
}

void ShotModel::onTimeChanged( int currentTime )
{
    int anticipatedLength = std::max( getMeasurement(), 5 ); // default to 5 until known
    int anticipatedSteps = 1 + (anticipatedLength << 2);
    int targetStepNo = anticipatedSteps * currentTime / mDuration;
    if ( targetStepNo == mLastStepNo ) {
        return;
    }

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

        bool hasTermination = hasTerminationPoint();
        do {
            if ( !hasTermination ) {
                if ( getAdjacentPosition( mLeadingDirection, &mLeadingPoint ) ) {
                    QPoint hitPoint = mLeadingPoint.toViewCenterSquare();
                    int entryDirection = mLeadingDirection;
                    if ( game.canShootThru( mLeadingPoint, &mLeadingDirection, nullptr, true, getShooter(), &hitPoint ) ) {
                        grow( mLeadingPoint.toViewCenterSquare(), entryDirection );
                        ++mDistance;
                    } else {
                        addTermination( entryDirection, hitPoint );
                        hasTermination = true;
                    }
                }
            }

            if ( mKillSequence ) {
                break;
            }

            if ( !mShedding
                // check if at max time before shedding (i.e. 3/4 of the anticipated steps)
              && ((mLastStepNo >= anticipatedSteps - anticipatedLength)
                // start shedding if terminated and sufficient time has elapsed
               || (hasTermination && currentTime >= SpeedController::HighSpeed - 30)) ) {
                mShedding = true;
            }

            if ( mShedding && !shedTail() ) {
                reset(); // done
                break;
            }
        } while( ++mLastStepNo < targetStepNo );
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

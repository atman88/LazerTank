#include "movecontroller.h"
#include "game.h"
#include "pathsearchaction.h"
#include "util/recorder.h"


MoveController::MoveController(QObject *parent) : QObject(parent), mState(Idle), mReplaySource(0)
{
}

void MoveController::init( Game* game )
{
    setParent(game);
    mFutureShots.setParent(game);

    QObject::connect( &game->getTank()->getShot(), &ShotModel::shooterReleased,      this, &MoveController::wakeup, Qt::QueuedConnection );
    QObject::connect( game->getShotAggregate(), &AnimationStateAggregator::finished, this, &MoveController::wakeup, Qt::QueuedConnection );
}

void MoveController::onBoardLoaded( Board* board )
{
    mToVector = ModelVector( board->getTankStartPoint(), 0 );

    mFutureShots.reset();
    mMoves.reset();

    // emit unconditionally to prime the pump:
    mState = Idle;
    emit idle();
}

void MoveController::move( int direction )
{
    std::cout << "MoveController: move " << direction << std::endl;
    if ( Game* game = getGame(this) ) {
        Tank* tank = game->getTank();
        Piece* lastMove = mMoves.getBack();
        ModelVector vector = (lastMove ? *lastMove : tank->getVector());

        if ( direction >= 0 && direction != vector.mAngle ) {
            if ( !lastMove || lastMove->hasPush() || lastMove->getShotCount()
               || (mState == FiringStage && mMoves.size() == 1) ) {
                appendMove( ModelVector(vector, direction) );
            } else {
                mMoves.replaceBack( MOVE_HIGHLIGHT, direction );
            }
        } else {
            Piece* pushPiece = 0;
            if ( game->canMoveFrom( TANK, vector.mAngle, &vector, true, &pushPiece ) ) {
                appendMove( vector, pushPiece );
                if ( pushPiece ) {
                    game->onFuturePush( pushPiece, vector.mAngle );
                }
            }
        }

        wakeup();
    }
}

void MoveController::fire( int count )
{
    std::cout << "MoveController: fire " << count << std::endl;
    if ( int nMoves = mMoves.size() ) {
        // -1 signifies an increment
        if ( count < 0 ) {
            count = mMoves.getBack()->getShotCount() + 1;
        }
        MovePiece* move = mMoves.setShotCountBack( count );
        // show its future if we expect it won't animate immediately:
        if ( count > 1 || mState == RotateStage ) {
            mFutureShots.updatePath( move );
        }
        if ( nMoves == 1 && mState == FiringStage ) {
            wakeup();
        }
    } else if ( count ) {
        if ( count < 0 ) {
            count = 1;
        }

        if ( Game* game = getGame(this) ) {
            Tank* tank = game->getTank();
            Piece* piece = mMoves.append( MOVE, tank->getVector(), count );
            // show it's future if it's got multiple shots
            if ( count > 1 ) {
                if ( MovePiece* move = dynamic_cast<MovePiece*>(piece) ) {
                    mFutureShots.updatePath( move );
                }
            }
            wakeup();
        }
    }
}

void MoveController::clearMoves()
{
    mMoves.reset();
}

void MoveController::wakeup()
{
    if ( Game* game = getGame(this) ) {
        Tank* tank = game->getTank();

        // always wait while the trigger is pressed:
        if ( tank->getShot().getShooter() ) {
            return;
        }

        // always wait for any current rotation to complete before proceeding with a queued move:
        if ( mState == RotateStage && game->getMoveAggregate()->active() ) {
            return;
        }

        Piece* move = mMoves.getFront();
        while( move ) {
            if ( mState != FiringStage ) {
                if ( !tank->getVector().equals( *move ) ) {
                    bool hasRotation = mToVector.mAngle != move->getAngle();
                    mToVector = *move;

                    if ( move->hasPush() ) {
                        emit pushingInto( move->getCol(), move->getRow(), move->getAngle() );
                    }

                    if ( tank->doMove( *move ) && hasRotation ) {
                        transitionState( RotateStage );
                        return; // waiting for current rotation to complete
                    }
                }
                transitionState( FiringStage );
            }

            if ( mState == FiringStage ) {
                if ( !move->getShotCount() ) {
                    if ( game->getMoveAggregate()->active() ) {
                        return; // waiting for current move to complete before advancing
                    }
                    transitionState( IdlingStage );
                } else {
                    if ( game->getShotAggregate()->active() ) {
                        return; // waiting for existing shot to complete
                    }
                    if ( tank->fire() ) {
                        if ( MovePiece* movePiece = dynamic_cast<MovePiece*>(move) ) {
                            movePiece->decrementShots();
                        }
                    }
                    return; // waiting for current shot to advance
                }
            }

            // nothing waiting on this move; advance to next move
            mFutureShots.removePath( move );
            mMoves.eraseFront();
            transitionState( IdlingStage );
            move = mMoves.getFront();
        }
    }

    transitionState( Idle );
}

void MoveController::eraseLastMove()
{
    if ( Piece* piece = mMoves.getBack() ) {
        mFutureShots.removePath( piece );
        mMoves.eraseBack();
        mMoves.replaceBack( MOVE_HIGHLIGHT );
    }
}

FutureShotPathManager* MoveController::getFutureShots()
{
    return &mFutureShots;
}

PieceListManager* MoveController::getMoves()
{
    return &mMoves;
}

void MoveController::onPathFound( PieceListManager* path, PathSearchAction* action )
{
    if ( action->getFocus() == TANK ) {
        mMoves.reset( path );
    } else {
        mMoves.replaceBack( MOVE );
        mMoves.append( *path->getList() );
    }
    mMoves.replaceBack( MOVE_HIGHLIGHT );

    if ( action->getMoveWhenFound() ) {
        wakeup();
    }
}

void MoveController::setReplay( bool on )
{
    if ( on ) {
        if ( !mReplaySource ) {
            mReplaySource = getGame(this)->getTank()->getRecorder().getReader();
            QObject::connect( this, &MoveController::idle, this, &MoveController::replayPlayback, Qt::QueuedConnection );
        }
    } else if ( mReplaySource ) {
        QObject::disconnect( this, &MoveController::idle, this, &MoveController::replayPlayback );
        getGame(this)->getTank()->getRecorder().closeReader();
        mReplaySource = 0;
        emit replayFinished();
    }
}

bool MoveController::replaying() const
{
    return mReplaySource != 0;
}

void MoveController::appendMove( ModelVector vector, Piece* pushPiece )
{
    mMoves.replaceBack( MOVE ); // erase highlight
    mMoves.append( MOVE_HIGHLIGHT, vector, 0, pushPiece );
}

void MoveController::transitionState(MoveState newState)
{
    if ( newState != mState ) {
        std::cout << "moveController ";
        switch( newState ) {
        case Idle:        std::cout << "Idle";        break;
        case RotateStage: std::cout << "RotateStage"; break;
        case FiringStage: std::cout << "FiringStage"; break;
        case IdlingStage: std::cout << "IdlingStage"; break;
        default:          std::cout << newState;      break;
        }
        std::cout << std::endl;

        mState = newState;
        if ( newState == Idle ) {
            emit idle();
        }
    }
}

void MoveController::setFocus( PieceType what )
{
    if ( mMoves.size() > 1 ) {
        mMoves.replaceBack( what == TANK ? MOVE : MOVE_HIGHLIGHT );
        mMoves.replaceFront(what == TANK ? MOVE_HIGHLIGHT : MOVE );
    }

    if ( Game* game = getGame(this) ) {
        if ( what == TANK ) {
            game->getTank()->pause();
        } else {
            game->getTank()->resume();
        }
    }
}

void MoveController::replayPlayback()
{
    if ( mReplaySource && mState == Idle ) {
        mReplaySource->readNext( this );
    }
}

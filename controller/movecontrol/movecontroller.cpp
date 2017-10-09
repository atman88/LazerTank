#include <iostream>
#include <stdint.h>
#include <QPainter>
#include "movecontroller.h"
#include "gameregistry.h"
#include "game.h"
#include "model/tank.h"
#include "view/boardrenderer.h"
#include "view/boardwindow.h"
#include "util/recorder.h"
#include "util/gameutils.h"

MoveController::MoveController(QObject *parent) : MoveDragController(parent), mReplayReader(0)
{
}

MoveController::~MoveController()
{
    if ( mReplayReader ) {
        delete mReplayReader;
    }
}

bool MoveController::canWakeup()
{
    if ( replaying() ) {
        return true;
    }
    return MoveDragController::canWakeup();
}

bool MoveController::setReplay( bool on )
{
    bool wasOn = (mReplayReader != 0);

    if ( on ) {
        if ( !mReplayReader ) {
            if ( GameRegistry* registry = getRegistry(this) ) {
                if ( RecorderSource* src = registry->getRecorder().source() ) {
                    if ( src->getReadState() == RecorderSource::Pending ) {
                        QObject::connect( src, &RecorderSource::dataReady, this, &MoveController::replayPlayback );
                    }
                    mReplayReader = new RecorderReader( registry->getGame().getBoard()->getTankStartVector().mAngle, *src );
                }
            }
            QObject::connect( this, &MoveController::idle, this, &MoveController::replayPlayback, Qt::QueuedConnection );
        }
    } else if ( mReplayReader ) {
        disconnect( this, SLOT(replayPlayback()) );
        delete mReplayReader;
        mReplayReader = 0;
        emit replayFinished();
    }

    return wasOn;
}

bool MoveController::replaying() const
{
    return mReplayReader != 0;
}

static QColor getTankShotColor( QObject* context )
{
    if ( GameRegistry* registry = getRegistry(context) ) {
        return registry->getTank().getShot().getPen().color();
    }
    return QColor(Qt::white); // should never happen - return something 'attention-getting'
}

void MoveController::render( Piece* pos, const QRect* rect, BoardRenderer& renderer, QPainter* painter )
{
    if ( !replaying() ) {
        // find the minimum shot path uid in the drag list:
        int minDragUID = 0x7fffffff;
        if ( getDragState() != Inactive ) {
            for( auto it : mDragMoves.getList() ) {
                if ( int uid = it->getShotPathUID() ) {
                    minDragUID = uid;
                    break;
                }
            }
        }

        QPen savePen = painter->pen();

        if ( mFutureShots.getPaths().size() ) {
            QPen pen( Qt::DashLine );
            for( auto it : mFutureShots.getPaths() ) {
                if ( rect->intersects( it.getBounds() ) ) {
                    QColor color = (it.getUID() >= minDragUID) ? getTankShotColor(this) : QColor(Qt::blue);
                    color.setAlpha(127); // dim it's color to contrast future shots from actual shots
                    pen.setColor( color );
                    pen.setWidth(2);
                    painter->setPen( pen );
                    painter->drawPath( *it.toQPath() );
                }
            }
            painter->setPen( savePen );
        }

        const PieceMultiSet* set = mMoves.toMultiSet();
        PieceSet::iterator it = set->lower_bound( pos );
        painter->setPen( Qt::blue );
        renderer.renderListIn( it, set->end(), rect, painter );

        painter->setPen( savePen );

        if ( getDragState() != Inactive && mDragMoves.size() ) {
            const PieceMultiSet* set = mDragMoves.toMultiSet();
            PieceSet::iterator it = set->lower_bound( pos );
            renderer.renderListIn( it, set->end(), rect, painter );
        }
    }
}

void MoveController::replayPlayback()
{
    if ( mReplayReader && mState == Idle ) {
        mReplayReader->consumeNext( this );
    }
}

int MoveDragController::getTileDragFocusAngle() const
{
    return mTileDragFocusAngle;
}

void MoveController::connectWindow( const BoardWindow* window ) const
{
    window->connectTo( mMoves );
    window->connectTo( mDragMoves );
}

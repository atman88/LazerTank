#include <QVector>

#include "boarddelta.h"
#include "board.h"

BoardDelta::BoardDelta(QObject *parent) : QObject(parent), mMasterBoard{nullptr}, mFutureBoard{nullptr}, mEnabled{false}
{
}

void BoardDelta::init( Board* masterBoard, Board* futureBoard )
{
    mMasterBoard = masterBoard;
    mFutureBoard = futureBoard;
}

const PieceSetManager& BoardDelta::getPieceManager() const
{
    return mPieceManager;
}

Board* BoardDelta::getFutureBoard()
{
    return mFutureBoard;
}

bool BoardDelta::enabled() const
{
    return mEnabled;
}

void BoardDelta::enable( bool newValue )
{
    if ( newValue != mEnabled ) {
        if ( !newValue ) {
            QObject::disconnect( &mMasterBoard->getPieceManager(), nullptr, this, nullptr );
            QObject::disconnect( &mFutureBoard->getPieceManager(), nullptr, this, nullptr );
            QObject::disconnect( mMasterBoard, &Board::tileChangedAt, this, nullptr );
            QObject::disconnect( mFutureBoard, &Board::tileChangedAt, this, nullptr );

            mPieceManager.reset(); // reset now for rendering
        } else {
            mFutureBoard->load( mMasterBoard );
            const PieceSetManager& masterManager = mMasterBoard->getPieceManager();
            PieceSetManager&       futureManager = mFutureBoard->getPieceManager();
            QObject::connect( &masterManager, &PieceSetManager::insertedAt, this, &BoardDelta::onChangeAt, Qt::DirectConnection );
            QObject::connect( &masterManager, &PieceSetManager::erasedAt,   this, &BoardDelta::onChangeAt, Qt::DirectConnection );
            QObject::connect( &futureManager, &PieceSetManager::insertedAt, this, &BoardDelta::onChangeAt, Qt::DirectConnection );
            QObject::connect( &futureManager, &PieceSetManager::erasedAt,   this, &BoardDelta::onChangeAt, Qt::DirectConnection );
            QObject::connect( mMasterBoard, &Board::tileChangedAt, this, &BoardDelta::onChangeAt, Qt::DirectConnection );
            QObject::connect( mFutureBoard, &Board::tileChangedAt, this, &BoardDelta::onChangeAt, Qt::DirectConnection );
        }
        mEnabled = newValue;
    }
}

void BoardDelta::onChangeAt( const ModelPoint& point )
{
    const Piece* masterPiece = mMasterBoard->getPieceManager().pieceAt( point );
    const Piece* futurePiece = mFutureBoard->getPieceManager().pieceAt( point );

    if ( !masterPiece ) {
        if ( !futurePiece ) {
            if ( mMasterBoard->tileAt(point) == mFutureBoard->tileAt(point) ) {
                // No discerned differences; Remove any existing delta:
                mPieceManager.eraseAt( point );
            } else {
                // No pieces, but tiles differ; indicate an erase:
                mPieceManager.setAt( TILE_FUTURE_ERASE, point, 0, mFutureBoard->getLastPushId() );
            }
        } else {
            // master=no future=yes; indicate an insert:
            mPieceManager.setAt( TILE_FUTURE_INSERT, point, 0, futurePiece->getPushedId() );
        }
    } else if ( !futurePiece ) {
        // master=yes future=no; indicate an erase:
        mPieceManager.setAt( TILE_FUTURE_ERASE, point, 0, mFutureBoard->getLastPushId() );
    } else {
        if ( masterPiece->getPushedId() != futurePiece->getPushedId() ) {
            // tile pushed to this point in the future
            mPieceManager.setAt( TILE_FUTURE_INSERT, point, 0, futurePiece->getPushedId() );
        } else if ( mMasterBoard->tileAt(point) == mFutureBoard->tileAt(point) ) {
            // No notable differences; Remove any existing delta:
            mPieceManager.eraseAt( point );
        } else {
            // tiles differ; indicate an erase:
            mPieceManager.setAt( TILE_FUTURE_ERASE, point, 0, futurePiece->getPushedId() );
        }
    }
}

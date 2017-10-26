#include <iostream>
#include <QVariant>
#include <QFile>
#include <QThread>
#include <QTextStream>

#include "board.h"
#include "controller/gameregistry.h"
#include "util/workerthread.h"

Board::Board( QObject* parent ) : QObject(parent), mLevel(0), mLastPushId(0), mStream(0)
{
    memset( mTiles, EMPTY, sizeof mTiles );
}

Board::~Board()
{
}

void Board::load( int level ) {
    QString namePattern( ":/maps/level%1.txt" );
    load( namePattern.arg(level), level );\
}

void Board::reload()
{
    if ( !mStream ) {
        load( mLevel );
    } else if ( mStream->seek(0) ) {
        load( *mStream, mLevel );
    }
}

PieceSetManager& Board::getPieceManager()
{
    return mPieceManager;
}

void Board::initPiece( PieceType type, int col, int row, int angle )
{
    mPieceManager.insert( type, ModelPoint(col,row), angle );
    mTiles[row*BOARD_MAX_HEIGHT + col] = DIRT;
}

int Board::getLastPushId() const
{
    return mLastPushId;
}

const ModelVector& Board::getTankStartVector() const
{
    return mTankWayPoint;
}

ModelPoint Board::getFlagPoint() const
{
    return mFlagPoint;
}

bool Board::load( const QString& fileName, int level )
{
    QFile file( fileName );
    bool rc = file.open(QIODevice::ReadOnly|QIODevice::Text);
    if ( rc ) {
        QTextStream stream(&file);
        load( stream, level );
        file.close();
    }
    return rc;
}

void Board::load( QTextStream& stream, int level )
{
    int row = 0;
    unsigned char* rowp = mTiles;
    mLowerRight = ModelPoint(0,0);
    mTankWayPoint = ModelVector(0,0);
    mFlagPoint.setNull();
    mPieceManager.reset();

    do {
        QString line = stream.readLine(BOARD_MAX_WIDTH);

        // we don't know the board width yet, so initialize the max:
        memset( rowp, EMPTY, BOARD_MAX_WIDTH * (sizeof *rowp) );

        int i = 0, col = 0;
        while( i < line.size() ) {
            QChar c = line.at(i++);
            if ( c.isSpace() ) {
                continue;
            }

            switch( c.unicode() ) {
            case 'S': rowp[col++] = STONE;     break;
            case 'W': rowp[col++] = WOOD;      break;
            case 'w': rowp[col++] = WATER;     break;
            case 'e': rowp[col++] = EMPTY;     break;
            case 'm': rowp[col++] = TILE_SUNK; break;
            case 'F':
                mFlagPoint.mCol = col;
                mFlagPoint.mRow = row;
                rowp[col++] = FLAG;
                break;

            case 'M': initPiece( TILE,   col++, row      ); break;
            case '^': initPiece( CANNON, col++, row      ); break;
            case '>': initPiece( CANNON, col++, row,  90 ); break;
            case 'v': initPiece( CANNON, col++, row, 180 ); break;
            case '<': initPiece( CANNON, col++, row, 270 ); break;
            case '[':
                if ( line.size()-i >= 2 ) {
                    int c1 = line.at(i++).unicode();
                    switch( (c1 << 8) | line.at(i++).unicode() ) {
                    case ('S' <<8)|'/':  rowp[col++] = STONE_MIRROR;     break;
                    case ('\\'<<8)|'S':  rowp[col++] = STONE_MIRROR__90; break;
                    case ('/' <<8)|'S':  rowp[col++] = STONE_MIRROR_180; break;
                    case ('S' <<8)|'\\': rowp[col++] = STONE_MIRROR_270; break;
                    case ('S' <<8)|'-':  rowp[col++] = STONE_SLIT;       break;
                    case ('S' <<8)|'|':  rowp[col++] = STONE_SLIT_90;    break;
                    case ('M' <<8)|'/':  initPiece( TILE_MIRROR, col++, row,   0 ); break;
                    case ('\\'<<8)|'M':  initPiece( TILE_MIRROR, col++, row,  90 ); break;
                    case ('/' <<8)|'M':  initPiece( TILE_MIRROR, col++, row, 180 ); break;
                    case ('M' <<8)|'\\': initPiece( TILE_MIRROR, col++, row, 270 ); break;
                    case ('T' << 8)|'^': mTankWayPoint = ModelVector( col, row,   0 ); rowp[col++] = DIRT;  break;
                    case ('T' << 8)|'>': mTankWayPoint = ModelVector( col, row,  90 ); rowp[col++] = DIRT;  break;
                    case ('T' << 8)|'v': mTankWayPoint = ModelVector( col, row, 180 ); rowp[col++] = DIRT;  break;
                    case ('T' << 8)|'<': mTankWayPoint = ModelVector( col, row, 270 ); rowp[col++] = DIRT;  break;
                    default:
                        ;
                    }
                }
                break;

            case 'T':
                mTankWayPoint = ModelVector( col, row );

                // fall through

            default:  rowp[col++] = DIRT;  break;
            }
        }
        if ( !col ) {
            break;
        }
        if ( col > mLowerRight.mCol ) {
            mLowerRight.mCol = col-1;
        }
        rowp += BOARD_MAX_WIDTH;
    } while( ++row < BOARD_MAX_HEIGHT );

    mLowerRight.mRow = row-1;
    mLevel = level;
    mLastPushId = 0;
    mStream = ( level < 0 ) ? &stream : 0;

    emit boardLoaded( level );
}

void Board::load( const Board* source )
{
    mLevel        = source->mLevel;
    mLastPushId   = source->mLastPushId;
    mLowerRight   = source->mLowerRight;
    mFlagPoint    = source->mFlagPoint;
    mTankWayPoint = source->mTankWayPoint;
    memcpy( mTiles, source->mTiles, sizeof mTiles );
    mPieceManager.reset( &source->mPieceManager );
    mStream = 0;
    emit boardLoaded( mLevel );
}

int Board::getWidth()
{
    return mLowerRight.mCol+1;
}

int Board::getHeight()
{
    return mLowerRight.mRow+1;
}

const ModelPoint& Board::getLowerRight() const
{
    return mLowerRight;
}

int Board::getLevel()
{
    return mLevel;
}

TileType Board::tileAt( ModelPoint point ) const {
    return (point.mCol >= 0 && point.mRow >= 0
         && point.mCol <= mLowerRight.mCol && point.mRow <= mLowerRight.mRow)
      ? ((TileType) mTiles[point.mRow*BOARD_MAX_WIDTH+point.mCol]) : EMPTY;
}

void Board::setTileAt( TileType id, ModelPoint point )
{
    if ( point.mCol >= 0 && point.mRow >= 0 && point.mCol <= mLowerRight.mCol && point.mRow <= mLowerRight.mRow ) {
        mTiles[point.mRow*BOARD_MAX_WIDTH+point.mCol] = id;
        emit tileChangedAt( point );
    }
}

void Board::applyPushResult( PieceType mType, ModelPoint point, int pieceAngle )
{
    ++mLastPushId;
    if ( tileAt( point ) != WATER ) {
        mPieceManager.insert( mType, point, pieceAngle, mLastPushId );
    } else if ( mType == TILE ) {
        setTileAt( TILE_SUNK, point );
    }
}

void Board::revertPush( MovePiece* pusher )
{
    ModelPoint point = *pusher;
    if ( getAdjacentPosition( pusher->getAngle(), &point ) ) {
        Piece* pushee = mPieceManager.pieceAt( point );
        int prevId = pusher->getPreviousPushedId();
        if ( pushee ) {
            if ( pushee->getPushedId() != mLastPushId ) {
                std::cout << "** revertPush: pushee out of order " << pushee->getPushedId() << " != " << mLastPushId << std::endl;
            }
            mPieceManager.erase( pushee );
        } else if ( pusher->getPushPieceType() == TILE && tileAt( point ) == TILE_SUNK ) {
            setTileAt( WATER, point );
        }
        --mLastPushId;
        mPieceManager.insert( pusher->getPushPieceType(), *pusher, pusher->getPushPieceAngle(), prevId );
    } else {
        std::cout << "** revertPush: failed to get adjacent pos for " << point.mCol << "," << point.mRow << std::endl;
    }
}

bool getAdjacentPosition( int angle, ModelPoint *point )
{
    switch( angle ) {
    case   0: point->mRow -= 1; return true;
    case  90: point->mCol += 1; return true;
    case 180: point->mRow += 1; return true;
    case 270: point->mCol -= 1; return true;
    default:
        ;
    }
    return false;
}

void Board::undoChanges( std::vector<FutureChange> changes )
{
    for( auto it = changes.end(); it != changes.begin(); ) {
        --it;
        switch( it->changeType ) {
        case TILE_CHANGE:
            setTileAt( it->u.tileType, it->point );
            break;

        case PIECE_ERASED:
            mPieceManager.insert( it->u.erase.pieceType, it->point, it->u.erase.pieceAngle, it->u.erase.previousPushedId );
            break;

        case PIECE_PUSHED:
        {   int reverseDirection = (it->u.multiPush.direction + 180) % 360;
            ModelPoint p = it->point;
            if ( Piece* pushee = mPieceManager.pieceAt( it->point ) ) {
                if ( pushee->getPushedId() != mLastPushId ) {
                    std::cout << "** undoChanges: pushee out of order " << pushee->getPushedId() << " != " << mLastPushId << std::endl;
                }
                mPieceManager.erase( pushee );
            } else {
                if ( it->u.multiPush.pieceType != TILE ) {
                    std::cout << "** undo PIECE_PUSHED didn't erase @ (" << p.mCol << "," << p.mRow << ")" << std::endl;
                } else if ( tileAt( it->point ) == TILE_SUNK ) {
                    setTileAt( WATER, it->point );
                } else {
                    std::cout << "** undo PIECE_PUSHED didn't erase @ non-sunk (" << p.mCol << "," << p.mRow << ")" << std::endl;
                }
            }
            mLastPushId -= it->u.multiPush.count;

            for( int i = it->u.multiPush.count; --i >= 0; ) {
                if ( !getAdjacentPosition( reverseDirection, &p ) ) {
                    std::cout << "** undo PIECE_PUSHED no adjacent at (" << p.mCol << "," << p.mRow << ")" << std::endl;
                    break;
                }
            }
            mPieceManager.insert( it->u.multiPush.pieceType, p, it->u.multiPush.pieceAngle, it->u.multiPush.previousPushedId );
        }
            break;

        default:
            ;
        }
    }
}


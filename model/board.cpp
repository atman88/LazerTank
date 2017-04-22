#include <iostream>
#include <QVariant>
#include <QFile>
#include <QThread>

#include "board.h"
#include "controller/gameregistry.h"
#include "util/workerthread.h"

class LoadRunnable : Runnable
{
public:
    LoadRunnable(Board& board) : mBoard(board)
    {
    }
    ~LoadRunnable()
    {
    }

    void loadFile( QString fileName, int level )
    {
        mFileName = fileName;
        mLevel = level;
        if ( GameRegistry* registry = getRegistry(&mBoard) ) {
            emit mBoard.boardLoading();
            registry->getWorker().doWork( this );
        }
    }

    void run() override
    {
        mBoard.load( mFileName, mLevel );
    }

private:
    Board& mBoard;
    QString mFileName;
    int mLevel;
};

Board::Board( QObject* parent ) : QObject(parent), mLevel(0), mWidth(6), mHeight(6), mStream(0), mRunnable(0)
{
    memset( mTiles, EMPTY, sizeof mTiles );
}

Board::~Board()
{
    delete mRunnable;
}

void Board::load( int level ) {
    QString namePattern( ":/maps/level%1.txt" );
    QString name = namePattern.arg(level);
    if ( !mRunnable ) {
        mRunnable = new LoadRunnable(*this);
    }
    mRunnable->loadFile( name, level );\
}

void Board::reload()
{
    if ( !mStream ) {
        load( mLevel );
    }

    if ( mStream->seek(0) ) {
        load( *mStream, mLevel );
    }
}

PieceSetManager& Board::getPieceManager()
{
    return mPieceManager;
}

void Board::initPiece( PieceType type, int col, int row, int angle )
{
    mPieceManager.insert( type, col, row, angle );
    mTiles[row*BOARD_MAX_HEIGHT + col] = DIRT;
}

ModelPoint Board::getTankStartPoint() const
{
    return mTankWayPoint;
}

ModelPoint Board::getFlagPoint() const
{
    return mFlagPoint;
}

bool Board::load( QString& fileName, int level )
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
    mWidth = 0;
    mTankWayPoint = ModelPoint(0,0);
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
                    default:
                        ;
                    }
                }
                break;

            case 'T':
                mTankWayPoint.mCol = col;
                mTankWayPoint.mRow = row;

                // fall through

            default:  rowp[col++] = DIRT;  break;
            }
        }
        if ( !col ) {
            break;
        }
        if ( col > mWidth ) {
            mWidth = col;
        }
        rowp += BOARD_MAX_WIDTH;
    } while( ++row < BOARD_MAX_HEIGHT );

    mHeight = row;
    mLevel = level;
    mStream = ( level < 0 ) ? &stream : 0;

    emit boardLoaded();
}

void Board::load( const Board* source )
{
    mLevel  = source->mLevel;
    mWidth  = source->mWidth;
    mHeight = source->mHeight;
    memcpy( mTiles, source->mTiles, sizeof mTiles );
    mPieceManager.reset( &source->mPieceManager );
    emit boardLoaded();
}

int Board::getWidth()
{
    return mWidth;
}

int Board::getHeight()
{
    return mHeight;
}

int Board::getLevel()
{
    return mLevel;
}

TileType Board::tileAt( int col, int row ) const {
    return (col >= 0 && row >= 0 && col < mWidth && row < mHeight) ? ((TileType) mTiles[row*BOARD_MAX_WIDTH+col]) : STONE;
}

void Board::setTileAt( TileType id, int col, int row )
{
    if ( col >= 0 && row >= 0 && col < mWidth && row < mHeight ) {
        mTiles[row*BOARD_MAX_WIDTH+col] = id;
        emit tileChangedAt( col, row );
    }
}

bool Board::applyPushResult( PieceType mType, int col, int row, int pieceAngle )
{
    if ( tileAt(col,row) != WATER ) {
        mPieceManager.insert( mType, col, row, pieceAngle );
        return true;
    }

    if ( mType == TILE ) {
        setTileAt( TILE_SUNK, col, row );
        return true;
    }

    return false;
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
            setTileAt( it->u.tileType, it->point.mCol, it->point.mRow );
            break;

        case PIECE_ERASED:
            mPieceManager.insert( it->u.erase.pieceType, it->point.mCol, it->point.mRow, it->u.erase.pieceAngle );
            break;

        case PIECE_PUSHED:
        {   int reverseDirection = (it->u.multiPush.direction + 180) % 360;
            ModelPoint p = it->point;
            if ( !mPieceManager.eraseAt( it->point.mCol, it->point.mRow ) ) {
                if ( it->u.multiPush.pieceType != TILE ) {
                    std::cout << "** undo PIECE_PUSHED didn't erase @ (" << p.mCol << "," << p.mRow << ")" << std::endl;
                } else if ( tileAt( it->point.mCol, it->point.mRow ) == TILE_SUNK ) {
                    setTileAt( WATER, it->point.mCol, it->point.mRow );
                } else {
                    std::cout << "** undo PIECE_PUSHED didn't erase @ non-sunk (" << p.mCol << "," << p.mRow << ")" << std::endl;
                }
            }

            for( int i = it->u.multiPush.count; --i >= 0; ) {
                if ( !getAdjacentPosition( reverseDirection, &p ) ) {
                    std::cout << "** undo PIECE_PUSHED no adjacent at (" << p.mCol << "," << p.mRow << ")" << std::endl;
                    break;
                }
            }
            mPieceManager.insert( it->u.multiPush.pieceType, p.mCol, p.mRow, it->u.multiPush.pieceAngle );
        }
            break;

        default:
            ;
        }
    }
}


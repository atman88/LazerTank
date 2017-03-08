#include <QVariant>
#include <QFile>

#include "board.h"

Board::Board( QObject* parent )  : QObject(parent), mLevel(0), mWidth(16), mHeight(16), mFlagCol(-1), mFlagRow(-1)
{
    memset( mTiles, EMPTY, sizeof mTiles );
}

bool Board::load( int level ) {
    QString namePattern( ":/maps/level%1.txt" );
    QString name = namePattern.arg(level);
    bool rc = load( name );\
    if ( rc ) {
        mLevel = level;
        emit boardLoaded();
    }
    return rc;
}

PieceSetManager* Board::getPieceManager()
{
    return &mPieceManager;
}

void Board::initPiece( PieceType type, int col, int row, int angle )
{
    mPieceManager.insert( type, col, row, angle );
    mTiles[row*BOARD_MAX_HEIGHT + col] = DIRT;
}

int Board::getTankStartCol() const
{
    return mTankWayPointCol;
}

int Board::getTankStartRow() const
{
    return mTankWayPointRow;
}

int Board::getFlagCol() const
{
    return mFlagCol;
}

int Board::getFlagRow() const
{
    return mFlagRow;
}

bool Board::load( QString& fileName )
{
    QFile file( fileName );
    if ( !file.open(QIODevice::ReadOnly|QIODevice::Text) ) {
        return false;
    }
    QTextStream stream(&file);
    load( stream );

    file.close();
    return true;
}

void Board::load( QTextStream& stream )
{
    int row = 0;
    unsigned char* rowp = mTiles;
    mWidth = 0;
    mTankWayPointCol = mTankWayPointRow = 0;
    mFlagCol = mFlagRow = -1;
    mPieceManager.reset();

    do {
        QString line = stream.readLine(BOARD_MAX_WIDTH);

        // we don't know the board width yet, so initialize the max:
        memset( rowp, EMPTY, BOARD_MAX_WIDTH * (sizeof *rowp) );

        int i = 0, col = 0;
        while( i < line.size() ) {
            while( line.at(i).isSpace() ) {
                ++i;
            }
            if ( i >= line.size() ) {
                break;
            }


            switch( line.at(i++).unicode() ) {
            case 'S': rowp[col++] = STONE;     break;
            case 'W': rowp[col++] = WOOD;      break;
            case 'w': rowp[col++] = WATER;     break;
            case 'e': rowp[col++] = EMPTY;     break;
            case 'm': rowp[col++] = TILE_SUNK; break;
            case 'F':
                mFlagCol = col;
                mFlagRow = row;
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
                mTankWayPointCol = col;
                mTankWayPointRow = row;

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
    mLevel = -1;
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

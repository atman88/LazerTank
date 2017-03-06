#include <QVariant>
#include <QFile>
#include "board.h"

Board::Board( QObject* parent )  : QObject(parent), mLevel(0), mWidth(16), mHeight(16), mFlagX(-1), mFlagY(-1)
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

void Board::initPiece( PieceType type, int x, int y, int angle )
{
    mPieceManager.insert( type, x, y, angle );
    mTiles[y*BOARD_MAX_HEIGHT + x] = DIRT;
}

int Board::getTankWayPointX() const
{
    return mTankWayPointX;
}

int Board::getTankWayPointY() const
{
    return mTankWayPointXY;
}

int Board::getFlagX() const
{
    return mFlagX;
}

int Board::getFlagY() const
{
    return mFlagY;
}

bool Board::load( QString& fileName )
{
    QFile file( fileName );
    if ( !file.open(QIODevice::ReadOnly|QIODevice::Text) ) {
        return false;
    }

    char line[BOARD_MAX_WIDTH];
    int y = 0;
    unsigned char* row = mTiles;
    mWidth = 0;
    mTankWayPointX = mTankWayPointXY = 0;
    mFlagX = mFlagY = -1;
    mPieceManager.reset();

    do {
        int nRead = file.readLine(line, sizeof line);
        memset( row, EMPTY, BOARD_MAX_WIDTH * (sizeof *row) );

        int i = 0, x = 0;
        while( i < nRead && x < BOARD_MAX_WIDTH ) {
            while( isspace(line[i]) ) {
                ++i;
            }

            switch( line[i++] ) {
            case 0:
                --i;
                break;
            case 'S': row[x++] = STONE;     break;
            case 'W': row[x++] = WOOD;      break;
            case 'w': row[x++] = WATER;     break;
            case 'e': row[x++] = EMPTY;     break;
            case 'm': row[x++] = TILE_SUNK; break;
            case 'F':
                mFlagX = x;
                mFlagY = y;
                row[x++] = FLAG;
                break;

            case 'M': initPiece( TILE,   x++, y      ); break;
            case '^': initPiece( CANNON, x++, y      ); break;
            case '>': initPiece( CANNON, x++, y,  90 ); break;
            case 'v': initPiece( CANNON, x++, y, 180 ); break;
            case '<': initPiece( CANNON, x++, y, 270 ); break;
            case '[':
                if ( nRead-i >= 2 ) {
                    int c1 = line[i++];
                    switch( (c1 << 8) | line[i++] ) {
                    case ('S' <<8)|'/':  row[x++] = STONE_MIRROR;     break;
                    case ('\\'<<8)|'S':  row[x++] = STONE_MIRROR__90; break;
                    case ('/' <<8)|'S':  row[x++] = STONE_MIRROR_180; break;
                    case ('S' <<8)|'\\': row[x++] = STONE_MIRROR_270; break;
                    case ('S' <<8)|'-':  row[x++] = STONE_SLIT;       break;
                    case ('S' <<8)|'|':  row[x++] = STONE_SLIT_90;    break;
                    case ('M' <<8)|'/':  initPiece( TILE_MIRROR, x++, y,   0 ); break;
                    case ('\\'<<8)|'M':  initPiece( TILE_MIRROR, x++, y,  90 ); break;
                    case ('/' <<8)|'M':  initPiece( TILE_MIRROR, x++, y, 180 ); break;
                    case ('M' <<8)|'\\': initPiece( TILE_MIRROR, x++, y, 270 ); break;
                    default:
                        ;
                    }
                }
                break;

            case 'T':
                mTankWayPointX = x;
                mTankWayPointXY = y;

                // fall through

            default:  row[x++] = DIRT;  break;
            }
        }
        if ( !x ) {
            break;
        }
        if ( x > mWidth ) {
            mWidth = x;
        }
        row += BOARD_MAX_WIDTH;
    } while( ++y < BOARD_MAX_HEIGHT );
    file.close();

    mHeight = y;
    mLevel = -1;

    return true;
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

TileType Board::tileAt( int x, int y ) const {
    return (x >= 0 && y >= 0 && x < mWidth && y < mHeight) ? ((TileType) mTiles[y*BOARD_MAX_WIDTH+x]) : STONE;
}

void Board::setTileAt( TileType id, int x, int y )
{
    if ( x >= 0 && y >= 0 && x < mWidth && y < mHeight ) {
        mTiles[y*BOARD_MAX_WIDTH+x] = id;
        emit tileChangedAt( x, y );
    }
}

bool Board::applyPushResult( PieceType mType, int x, int y, int pieceAngle )
{
    if ( tileAt(x,y) != WATER ) {
        mPieceManager.insert( mType, x, y, pieceAngle );
        return true;
    }

    if ( mType == TILE ) {
        setTileAt( TILE_SUNK, x, y );
        return true;
    }

    return false;
}

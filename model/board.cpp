#include <QVariant>
#include <QFile>
#include "board.h"


Board::Board( QObject* parent )  : QObject(parent)
{
    mLevel = 0;
    mWidth = 16;
    mHeight = 16;
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

bool Board::load( QString& fileName )
{
    QFile file( fileName );
    if ( !file.open(QIODevice::ReadOnly|QIODevice::Text) ) {
        return false;
    }

    char line[BOARD_MAX_WIDTH];
    int y = 0;
    mWidth = 0;
    mInitialTankX = mInitialTankY = 0;
    mPieces.clear();
    memset( mTiles, EMPTY, sizeof mTiles );

    do {
        int nRead = file.readLine(line, sizeof line);

        int i = 0, x = 0;
        while( i < nRead ) {
            while( isspace(line[i]) ) {
                ++i;
            }

            switch( line[i++] ) {
            case 0:
                --i;
                break;
            case 'S': mTiles[y*BOARD_MAX_WIDTH + x++] = STONE;     break;
            case 'w': mTiles[y*BOARD_MAX_WIDTH + x++] = WATER;     break;
            case 'e': mTiles[y*BOARD_MAX_WIDTH + x++] = EMPTY;     break;
            case 'F': mTiles[y*BOARD_MAX_WIDTH + x++] = FLAG;      break;
            case 'm': mTiles[y*BOARD_MAX_WIDTH + x++] = TILE_SUNK; break;
            case 'M':
                mPieces.insert( Piece( TILE, x, y ) );
                mTiles[y*BOARD_MAX_HEIGHT + x++] = DIRT;
                break;
            case '[':
                if ( nRead-i >= 2 ) {
                    int c1 = line[i++];
                    switch( (c1 << 8) | line[i++] ) {
                    case ('S' <<8)|'/':  mTiles[y*BOARD_MAX_WIDTH + x++] = STONE_MIRROR___0; break;
                    case ('\\'<<8)|'S':  mTiles[y*BOARD_MAX_WIDTH + x++] = STONE_MIRROR__90; break;
                    case ('/' <<8)|'S':  mTiles[y*BOARD_MAX_WIDTH + x++] = STONE_MIRROR_180; break;
                    case ('S' <<8)|'\\': mTiles[y*BOARD_MAX_WIDTH + x++] = STONE_MIRROR_270; break;
                    case ('S' <<8)|'-':  mTiles[y*BOARD_MAX_WIDTH + x++] = STONE_SLIT__0; break;
                    case ('S' <<8)|'|':  mTiles[y*BOARD_MAX_WIDTH + x++] = STONE_SLIT_90; break;
                    default:
                        ;
                    }
                }
                break;

            case 'T':
                mInitialTankX = x;
                mInitialTankY = y;

                // fall through

            default:  mTiles[y*BOARD_MAX_WIDTH + x++] = DIRT;  break;
            }

            if ( x >= BOARD_MAX_WIDTH ) {
                break;
            }
        }
        if ( !x ) {
            break;
        }
        if ( x > mWidth ) {
            mWidth = x;
        }
    } while( ++y < BOARD_MAX_HEIGHT );
    file.close();

    mHeight = y;
    setProperty( "tiles", QVariant::fromValue(mPieces) );

    return true;
}

PieceType Board::pieceAt( int x, int y )
{
    Piece pos( NONE, x, y );
    PieceSet::iterator it = mPieces.find( pos );
    if ( it != mPieces.end() ) {
        return it->getType();
    }
    return NONE;
}

void Board::erasePieceAt( int x, int y )
{
    Piece pos( NONE, x, y );
    PieceSet::iterator it = mPieces.find( pos );
    if ( it != mPieces.end() ) {
        mPieces.erase( it );
        setProperty( "tiles", QVariant::fromValue(mPieces) );
    }
}

void Board::addPiece( PieceType type, int x, int y )
{
    mPieces.insert( Piece( type, x, y ) );
    setProperty( "tiles", QVariant::fromValue(mPieces) );
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

BoardTileId Board::tileAt( int x, int y ) {
    return (x >= 0 && y >= 0 && x < mWidth && y < mHeight) ? mTiles[y*BOARD_MAX_WIDTH+x] : STONE;
}

void Board::setTileAt( BoardTileId id, int x, int y )
{
    if ( x >= 0 && y >= 0 && x < mWidth && y < mHeight ) {
        mTiles[y*BOARD_MAX_WIDTH+x] = id;
        emit tileChanged( x, y );
    }
}

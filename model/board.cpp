#include <QFile>
#include "board.h"


Board::Board( const string& fileName ) {
    mWidth = 16;
    mHeight = 16;
    load( fileName );
}

void Board::load( const string& fileName ) {
    char line[BOARD_MAX_WIDTH];
    int y = 0;
    mWidth = 0;
    mInitialTankX = mInitialTankY = 0;

    QString qname(fileName.c_str());
    QFile file( qname );
    if ( file.open(QIODevice::ReadOnly|QIODevice::Text) ) {
        bool done = false;
        do {
            int nRead = file.readLine(line, sizeof line);
            done = nRead < 0;
            int x;
            for( x = 0; x < nRead; ++x ) {
                switch( line[x] ) {
                case 'S': mTiles[y*BOARD_MAX_HEIGHT+x] = STONE; break;
                case 'w': mTiles[y*BOARD_MAX_HEIGHT+x] = WATER; break;
                case 'T':
                    mInitialTankX = x;
                    mInitialTankY = y;

                    // fall through

                default:  mTiles[y*BOARD_MAX_HEIGHT+x] = DIRT;  break;
                }
            }
            if ( x > mWidth ) {
                mWidth = x;
            }

            if ( ++y >= BOARD_MAX_HEIGHT ) {
                break;
            }
        } while( !done );
        file.close();
    }
    mHeight = y;
}

int Board::getWidth()
{
    return mWidth;
}

int Board::getHeight()
{
    return mHeight;
}

BoardTileId Board::tileAt( int x, int y ) {
    return (x >= 0 && y >= 0 && x < mWidth && y < mHeight) ? mTiles[y*BOARD_MAX_WIDTH+x] : STONE;
}


#include <iostream>
#include <string>
#include <QPixmap>
#include <QPainter>

#include "imageutils.h"
#include "model/board.h"

ResourcePixmap* ResourcePixmap::nameArray[PixmapTypeUpperBound] = { nullptr };
ResourcePixmap* ResourcePixmap::NullPixmap = nullptr;
std::map<int,ResourcePixmap> ResourcePixmap::nameMap;

ResourcePixmap* ResourcePixmap::getNullPixmap()
{
    if ( !NullPixmap ) {
        NullPixmap = new ResourcePixmap("null");
    }
    return NullPixmap;
}

const ResourcePixmap* ResourcePixmap::getPixmap( unsigned type )
{
    if ( type >= (sizeof nameArray)/(sizeof *nameArray) ) {
        std::cout << "attempt to get INVALID pixmap " << type << std::endl;
        return getNullPixmap();
    }

    ResourcePixmap *p = nameArray[type];
    if ( !p ) {
        if ( nameMap.empty() ) {
            nameMap = {
                { STONE,             { "wall-stone"          } },
                { DIRT,              { "dirt"                } },
                { WATER,             { "water"               } },
                { TILE_SUNK,         { "tile-sunk"           } },
                { FLAG,              { "dirt+flag"           } },
                { TILE,              { "tile-metal"          } },
                { MOVE,              { "c&move-indicator"    } },
                { MOVE_HIGHLIGHT,    { "c&move-highlight"    } },
                { STONE_MIRROR,      { "dirt+wall-mirror"    } },
                { STONE_SLIT,        { "stone-slit"          } },
                { WOOD,              { "wood"                } },
                { WOOD_DAMAGED,      { "wood-damaged"        } },
                { TILE_MIRROR,       { "tile-mirror"         } },
                { CANNON,            { "cannon"              } },
                { TANK,              { "tank"                } },
                { TILE_FUTURE_ERASE, { "c&tile-future-erase" } },
                { TILE_FUTURE_INSERT,{ "c&tile-future-insert"} },
                { TANK_FAST,         { "tank-fast"           } },
                { COMPLETE_CHECKMARK,{"complete-checkmark"   } }
            };
        }

        auto it = nameMap.find( type );
        if ( it == nameMap.end() ) {
            p = getNullPixmap();
        } else {
            p = &it->second;

            p->load();
        }
        nameArray[type] = p;
    }
    return p;
}

ResourcePixmap::ResourcePixmap( const char* name ) : mName(name), mTagCount(0), mBluePixmap(nullptr)
{
    if ( mName[1] == '&' ) {
        mTagCount = 1;
        mName += 2;
    }
}

ResourcePixmap::~ResourcePixmap()
{
    delete mBluePixmap;
}

bool ResourcePixmap::load()
{
    const char* name = mName;
    const char* secondName = strchr( name, '+' );
    if ( secondName ) {
        ++secondName;
        auto nameSize = static_cast<size_t>( secondName - mName );
        auto nameBuf = static_cast<char*>( alloca( nameSize ) );
        strncpy( nameBuf, name, nameSize );
        nameBuf[nameSize-1] = 0;
        name = nameBuf;
    }

    QString path = QString( ":/images/%1.png" ).arg( name );
    if ( !QPixmap::load( path ) ) {
        std::cout << "** failed to load " << qPrintable(path) << std::endl;
        return false;
    }

    if ( secondName ) {
        QPixmap pixmap2;
        path = QString( ":/images/%1.png" ).arg( secondName );
        if ( !pixmap2.load( path ) ) {
            std::cout << "** failed to load " << qPrintable(path) << std::endl;
        } else {
            QPainter painter( this );
            painter.drawPixmap( (width()-pixmap2.width())/2, (height()-pixmap2.height())/2, pixmap2 );
        }
    }

    if ( hasColorableTag() ) {
        QImage image = toImage();
        int width = image.width();
        for( int y = image.height(); --y >= 0; ) {
            for( int x = width; --x >= 0; ) {
                // swap green <-> blue
                QRgb p = image.pixel( x, y );
                image.setPixel( x, y, (p & 0xffff0000) | (qBlue(p) << 8) | qGreen(p) );
            }
        }
        mBluePixmap = new QPixmap( QPixmap::fromImage( image ) );
    }
    return true;
}

bool ResourcePixmap::hasColorableTag() const
{
    // just test the count given it's the only tag currently defined
    return mTagCount > 0;
}

const char* ResourcePixmap::getName() const
{
    return mName;
}

const QPixmap* ResourcePixmap::getForColor( const QColor& color ) const
{
    static const QRgb blueRgb = QColor( Qt::blue ).rgb();

    if ( mBluePixmap && color.rgb() == blueRgb ) {
        return mBluePixmap;
    }
    return this;
}

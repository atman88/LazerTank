#include <iostream>
#include <map>
#include <string>

#include "imageutils.h"
#include "model/board.h"

class NamedPixmap : public QPixmap
{
public:
    NamedPixmap()
    {
    }

    NamedPixmap(const char* name) : mName(name)
    {
    }

    ~NamedPixmap()
    {
    }

    const char* mName;
};

const QPixmap* getPixmap( unsigned type )
{
    static std::map<int,NamedPixmap> nameMap = {
        { STONE,             { "wall-stone"         } },
        { DIRT,              { "dirt"               } },
        { TILE_SUNK,         { "tile-sunk"          } },
        { FLAG,              { "flag"               } },
        { TILE,              { "tile-metal"         } },
        { MOVE,              { "move-indicator"     } },
        { SHOT_STRAIGHT,     { "shot-straight"      } },
        { SHOT_RIGHT,        { "shot-right"         } },
        { SHOT_END,          { "shot-end"           } },
        { STONE_MIRROR,      { "wall-mirror"        } },
        { STONE_SLIT,        { "stone-slit"         } },
        { WOOD,              { "wood"               } },
        { WOOD_DAMAGED,      { "wood-damaged"       } },
        { TILE_MIRROR,       { "tile-mirror"        } },
        { CANNON,            { "cannon"             } },
        { TANK,              { "tank"               } },
        { TILE_FUTURE_ERASE, { "tile-future-erase"  } },
        { TILE_FUTURE_INSERT,{ "tile-future-insert" } }
    };
    static NamedPixmap* nameArray[TileTypeUpperBound] = { 0 };
    static QString pathFormat( ":/images/%1.png" );
    static NamedPixmap NullPixmap( "null" );

    if ( type >= (sizeof nameArray)/(sizeof *nameArray) ) {
        cout << "attempt to get INVALID pixmap " << type << std::endl;
        return &NullPixmap;
    }

    NamedPixmap *p = nameArray[type];
    if ( !p ) {
        auto it = nameMap.find( type );
        if ( it == nameMap.end() ) {
            cout << "didn't' find pixmap " << type << std::endl;
            p = &NullPixmap;
        } else {
            p = &it->second;
            p->load( pathFormat.arg( p->mName ) );
            if ( p->isNull() ) {
                cout << "failed to load " << pathFormat.arg( p->mName ).toStdString() << std::endl;
            }
        }
        nameArray[type] = p;
    }
    return p;
}

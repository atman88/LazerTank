#ifndef IMAGEUTILS_H
#define IMAGEUTILS_H

#include <map>
#include <QPixmap>

#include "view/pieceview.h"

typedef enum {
    TANK_FAST = PieceTypeUpperBound,
    COMPLETE_CHECKMARK,
    PixmapTypeUpperBound // must be last
} PixmapType;

class ResourcePixmap : public QPixmap
{
public:
    ResourcePixmap( const char* getName );
    ~ResourcePixmap();

    /**
     * @brief Retrieve the image for the given piece or tile type
     * @param type Identifier. Can be a PieceType, TileType or PixmapType value.
     * @return The associated pixmap. An empty pixmap is returned for unregistered types.
     */
    static const ResourcePixmap* getPixmap( unsigned type );

    bool load();

    /**
     * @brief Query if this pixmap has been tagged as 'colorable'. I.e. it's one that's meant to have it's color manipulated
     */
    bool hasColorableTag() const;

    const char* getName() const;

    const QPixmap* getForColor( const QColor& color ) const;

private:
    static ResourcePixmap* getNullPixmap();

    const char* mName;
    int mTagCount;
    QPixmap* mBluePixmap;

    static std::map<int,ResourcePixmap> nameMap;
    static ResourcePixmap* nameArray[PixmapTypeUpperBound];
    static ResourcePixmap* NullPixmap;
};

#endif // IMAGEUTILS_H

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

/**
 * @brief A resource-based pixmap
 */
class ResourcePixmap : public QPixmap
{
public:
    ResourcePixmap( const char* name );
    ~ResourcePixmap();

    /**
     * @brief Retrieve the image for the given piece or tile type
     * @param type Identifier. Can be a PieceType, TileType or PixmapType value.
     * @return The associated pixmap. An empty pixmap is returned for unregistered types.
     */
    static const ResourcePixmap* getPixmap( unsigned type );

    /**
     * @brief Load this pixmap unconditionally
     * @return true if successful
     */
    bool load();

    /**
     * @brief Query if this pixmap has been tagged as 'colorable'. I.e. it's one that's meant to have it's color manipulated
     */
    bool hasColorableTag() const;

    /**
     * @brief Get the name of this pixmap
     */
    const char* getName() const;

    /**
     * @brief Get the pixmap associated with the given color
     * @param color Key value
     * @return The associated pixmap, or the default pixmap if no association for the given color
     */
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

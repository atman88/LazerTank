#ifndef PIECESETMANAGER_H
#define PIECESETMANAGER_H

#include <QObject>

#include "piece.h"

/**
 * @brief Manages a self-contained set of pieces
 */
class PieceSetManager : public PieceManager
{
    Q_OBJECT

public:
    explicit PieceSetManager( QObject* parent = 0 );
    ~PieceSetManager();

    /**
     * @brief Get the underlying set being managed
     */
    const PieceSet& getPieces() const;

    /**
     * @brief Creates a new piece from the given values and adds it to this set
     */
    void insert( PieceType type, ModelPoint point, int angle = 0, int pushedId = 0 );

    /**
     * @brief Creates a copy of the given piece and adds it to this set
     */
    void insert( Piece* piece );

    /**
     * @brief Searches for a piece in this set that has the given position
     * @param point The position to search for
     * @return The type of piece at the given position, or NONE if not found
     */
    PieceType typeAt( ModelPoint point );

    /**
     * @brief Searches for a piece in this set that has the given position
     * @param point position to search for
     * @return The piece at the given position, or 0 if not found
     */
    Piece* pieceAt( ModelPoint point ) const;

    /**
     * @brief removes any piece from the set at the postion specified by key
     * @return true if the piece was removed
     */
    bool erase( Piece* key );

    /**
     * @brief removes any piece from the set at the specified postion
     * @return true if the piece was removed
     */
    bool eraseAt( ModelPoint point );

    /**
     * @brief Add or change the piece at the given square
     * Any existing piece at the given square is changed, otherwise a piece is added.
     * @param point The target square
     * @param type The type to set or add
     * @param angle The piece rotation to set or add
     * @param pushedId If non-zero, associate the given pushedId
     */
    void setAt( PieceType type, ModelPoint point, int angle = 0, int pushedId = 0 );

    /**
     * @brief Re-initialize the set
     * @param source If non-zero, initialize with a copy of the given source, otherwise the set is cleared
     */
    void reset( const PieceSetManager* source = 0 );

    /**
     * @brief Query the number of pieces in the list
     * @return the number of pieces
     */
    int size() const;

public slots:
    void invalidatePushIdDelineation( int delineation );

private:
    PieceSet mPieces;
};

#endif // PIECESETMANAGER_H

#ifndef PIECESETMANAGER_H
#define PIECESETMANAGER_H

#include <QObject>

#include "piece.h"

/**
 * @brief Manages a self-contained set of pieces
 */
class PieceSetManager : public QObject
{
    Q_OBJECT

public:
    explicit PieceSetManager( QObject* parent = 0 );
    ~PieceSetManager();

    /**
     * @brief Get the underlying set being managed
     */
    const PieceSet* getPieces() const;

    /**
     * @brief Creates a new piece from the given values and adds it to this set
     */
    void insert( PieceType type, int col, int row, int angle = 0 );

    /**
     * @brief Creates a copy of the given piece and adds it to this set
     */
    void insert( Piece* piece );

    /**
     * @brief Searches for a piece in this set that has the given position
     * @param col Column position to search for
     * @param row Row position to search for
     * @return The type of piece at the given position, or NONE if not found
     */
    PieceType typeAt( int col, int row );

    /**
     * @brief Searches for a piece in this set that has the given position
     * @param col Column position to search for
     * @param row Row position to search for
     * @return The piece at the given position, or 0 if not found
     */
    Piece* pieceAt(int col, int row ) const;

    /**
     * @brief removes any piece from the set at the postion specified by key
     * @return true if the piece was removed
     */
    bool erase( Piece* key );

    /**
     * @brief removes any piece from the set at the specified postion
     * @return true if the piece was removed
     */
    bool eraseAt( int col, int row );

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

signals:
    /**
     * @brief Notifies that a new piece was added to the set
     * @param col The column of the new piece
     * @param row The row of the new piece
     */
    void insertedAt( int col, int row );

    /**
     * @brief Notifies that a piece was deleted from the set
     * @param col The column of the deleted piece
     * @param row The row of the deleted piece
     */
    void erasedAt( int col, int row );

private:
    PieceSet mPieces;
};

#endif // PIECESETMANAGER_H

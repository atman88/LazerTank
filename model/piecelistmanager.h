#ifndef PIECELISTMANAGER_H
#define PIECELISTMANAGER_H

#include <QObject>

#include "piece.h"

/**
 * @brief A manager for piece lists
 */
class PieceListManager : public QObject
{
    Q_OBJECT

public:
    explicit PieceListManager(QObject *parent = 0);
    ~PieceListManager();

    /**
     * @brief Get the underlying list being managed
     */
    const PieceList& getList() const;

    /**
     * @brief Get this list in the form of a set
     * Note that duplicates (i.e. more than one piece at the same square position) will not be contained in the returned set.
     * @return The set of pieces
     */
    const PieceSet* toSet();

    /**
     * @brief Get this list in the form of a multiset
     * @return The multiset of pieces
     */
    const PieceMultiSet* toMultiSet();

    /**
     * @brief Adds a new piece to the end of this list from the given values
     */
    Piece* append( PieceType type, ModelPoint point, int angle = 0 );
    Piece* append( PieceType type, ModelPoint point, int angle, int shotCount, const Piece* pushPiece = 0 );
    Piece* append( PieceType type, ModelVector vector, int shotCount, const Piece* pushPiece = 0 );

    /**
     * @brief Adds a copy of the given piece to the end of this list
     * @param source The piece to append
     */
    Piece* append( const Piece* source );

    /**
     * @brief Copy elements from source into the end of this list
     * @param source The pieces to append
     */
    void append( const PieceList& source );

    /**
     * @brief Adds a new piece to the beginning of this list from the given values
     */
    Piece* push_front( PieceType type, ModelVector vector, int shotCount = 0, const Piece* pushPiece = 0 );

    /**
     * @brief get the first element
     * @return the last element in the list, or 0 if empty
     */
    Piece* getFront() const;

    /**
     * @brief get the last element
     * @return the last element in the list, or 0 if empty
     */
    Piece* getBack() const;

    /**
     * @brief Get the nth piece from the back of the list
     * @param index The reverse offset of the piece from the back of the list
     * @return The selected piece or 0 if not present
     */
    Piece* getBack( int index ) const;

    /**
     * @brief removes the first element
     * @return true if the element was removed
     */
    bool eraseFront();

    /**
     * @brief removes the last element
     * @return true if the element was removed
     */
    bool eraseBack();

    /**
     * @brief Modifies the first element in the list with the given values
     * @return true if the piece was changed
     */
    bool replaceFront( PieceType type, int newAngle = -1 );

    /**
     * @brief Modifies the last element in the list with the given values
     * @return true if the piece was changed
     */
    bool replaceBack( PieceType type, int newAngle = -1 );

    /**
     * @brief Set the number of future shots on the last element
     * @return the updated element or 0 if not updated
     */
    MovePiece* setShotCountBack( int count );

    /**
     * @brief Query the number of pieces in the list
     * @return the number of pieces
     */
    int size() const;

signals:
    /**
     * @brief Notifies that a new piece was added at the end of the list
     * @param point The associated square of the new piece
     */
    void added( ModelPoint point );
    /**
     * @brief Notifies that a piece was deleted from the list
     * @param point The square of the deleted piece
     */
    void erased( ModelPoint point );
    /**
     * @brief Notifies that a piece in the list was modified
     * @param point The square of the modified piece
     */
    void changed( ModelPoint point );

public slots:
    /**
     * @brief Re-initialize the list
     * @param source If non-zero, initialize with a copy of the given source, otherwise clear the list
     */
    void reset( PieceListManager* source = 0 );

private:
    Piece* addInternal( Piece* piece, bool pushFront = false );
    bool eraseInternal(PieceList::iterator it );
    void replaceInternal( Piece* piece, PieceType type, int newAngle );

    PieceList mPieces;
    PieceSet* mSet;
    PieceMultiSet* mMultiSet;
};

#endif // PIECELISTMANAGER_H

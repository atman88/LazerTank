#ifndef PIECESETMANAGER_H
#define PIECESETMANAGER_H

#include <QObject>

#include "piece.h"


class PieceSetManager : public QObject
{
    Q_OBJECT

public:
    explicit PieceSetManager( QObject* parent = 0 );
    ~PieceSetManager();

    const PieceSet* getPieces() const;
    void insert( PieceType type, int x, int y, int angle = 0 );
    void insert( Piece* piece );
    PieceType typeAt( int x, int y );
    Piece* pieceAt(int x, int y ) const;
    bool erase( Piece* key );
    bool eraseAt( int x, int y );
    void reset( const PieceSetManager* source = 0 );
    int getLastTransactionNo();
    int count() const;

signals:
    void insertedAt( int x, int y );
    void erasedAt( int x, int y );

private:
    int mLastTransactionNo;
    PieceSet mPieces;
};

#endif // PIECESETMANAGER_H

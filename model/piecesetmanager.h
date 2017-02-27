#ifndef PIECESETMANAGER_H
#define PIECESETMANAGER_H

#include <QObject>

#include "piece.h"


class PieceSetManager : public QObject
{
    Q_OBJECT

public:
    explicit PieceSetManager( QObject* parent = 0 );

    const PieceSet* getPieces() const;
    void insert( PieceType type, int x, int y, int angle );
    void insert( Piece* piece );
    PieceType typeAt( int x, int y );
    Piece *pieceAt(int x, int y );
    void erase( Piece* key );
    void eraseAt( int x, int y );
    void reset( PieceSetManager* source = 0 );
    int getLastTransactionNo();
    int count() const;

signals:
    void insertedAt( int x, int y );
    void erasingAt( int x, int y );

private:
    int mLastTransactionNo;
    PieceSet mPieces;
};

#endif // PIECESETMANAGER_H

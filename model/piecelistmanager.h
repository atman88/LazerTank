#ifndef PIECELISTMANAGER_H
#define PIECELISTMANAGER_H

#include <QObject>

#include "piece.h"

class PieceListManager : public QObject
{
    Q_OBJECT

public:
    explicit PieceListManager(QObject *parent = 0);
    ~PieceListManager();

    const PieceList* getList() const;
    const PieceSet* toSet();
    const PieceMultiSet* toMultiSet();
    void append( PieceType type, int x, int y, int angle = 0, bool hasPush = false );
    void append( PieceType type, int x, int y, int angle, int pusheeOffset );
    void append( PieceType type, int x, int y, int angle, QColor* color );
    void append( PieceType type, int x, int y, int angle, int pusheeOffset, QColor* color );
    void append(Piece* source );
    bool eraseFront();
    bool eraseBack();
    bool replaceBack( PieceType type, int newAngle = -1 );
    int count() const;

signals:
    void appended( int x, int y );
    void erased( int x, int y );
    void replaced( int x, int y );

public slots:
    void reset( PieceListManager* source = 0 );

private:
    void appendInternal( Piece* piece );
    bool eraseInternal(PieceList::iterator it );

    PieceList mPieces;
    PieceSet* mSet;
    PieceMultiSet* mMultiSet;
};

#endif // PIECELISTMANAGER_H

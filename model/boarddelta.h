#ifndef BOARDDELTA_H
#define BOARDDELTA_H

#include <QObject>

class Board;

#include "piecesetmanager.h"

/**
 * @brief A class which compares two boards
 */
class BoardDelta : public QObject
{
    Q_OBJECT

public:
    explicit BoardDelta( QObject *parent = nullptr );

    /**
     * @brief Initialization method. Specifies two boards to compare.
     * @param masterBoard The first board to compare against
     * @param futureBoard The second board to compare with
     */
    void init( Board* masterBoard, Board* futureBoard );

    /**
     * @brief Getter method for retrieving the second board being used for comparison
     */
    Board* getFutureBoard();

    /**
     * @brief Access the pieces which depict the differences between the two boards
     */
    const PieceSetManager& getPieceManager() const;

    /**
     * @brief Query whether delta tracking is currently enabled
     * @return
     */
    bool enabled() const;

    /**
     * @brief Turn on/off delta tracking
     * @param newValue true to turn on tracking
     */
    void enable( bool newValue = true );

public slots:
    /**
     * @brief Receives changes from the two boards
     * @param point The square associated with the change
     */
    void onChangeAt( const ModelPoint& point );

private:
    PieceSetManager   mPieceManager;
    Board*  mMasterBoard;
    Board*  mFutureBoard;
    bool mEnabled;
};

#endif // BOARDDELTA_H

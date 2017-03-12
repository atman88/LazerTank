#ifndef GAMEUTILS_H
#define GAMEUTILS_H

#include <QObject>

class Game;

/**
 * @brief The Game handle
 * The QObject property referencing this Game object
 */
struct GameHandle
{
    Game* game;
};
Q_DECLARE_METATYPE(GameHandle)


/**
 * @brief Obtain the Game object from a given object's parental hierarchy
 * @param gameObject A QObject associated with the desired Game
 * @return The Game if found, otherwise 0
 */
extern Game* getGame( QObject* gameObject );

/**
 * @brief convert a model coordinate into a view center point
 * @param col
 * @param row
 * @return The view center point
 */
extern QPoint modelToViewCenterSquare( int col, int row );

/**
 * @brief Convert a view point that currently represents the center of a square to a the point where a shot would
 * the square
 * @param angle Entry direction
 * @param point Input as a center point; Returns the relative associated entry point
 */
extern void centerToEntryPoint( int angle, QPoint *point );

#endif // GAMEUTILS_H

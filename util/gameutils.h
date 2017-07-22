#ifndef GAMEUTILS_H
#define GAMEUTILS_H

#include <QObject>

class GameRegistry;

/**
 * @brief The game handle property identifier name
 */
extern const char* GameHandleName;

/**
 * @brief The Game handle
 * The QObject property referencing this Game object
 */
struct GameHandle
{
    GameRegistry* registry;
};
Q_DECLARE_METATYPE(GameHandle)


/**
 * @brief Obtain the game registry from a given object's parental hierarchy
 * @param gameObject A QObject associated with the game hierarchy
 * @return The registry if found, otherwise 0
 */
extern GameRegistry* getRegistry( const QObject* gameObject );

/**
 * @brief Convert a view point that currently represents the center of a square to a the point where a shot would
 * the square
 * @param angle Entry direction
 * @param point Input as a center point; Returns the relative associated entry point
 */
extern void centerToEntryPoint( int angle, QPoint *point );

#endif // GAMEUTILS_H

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
 * @brief Obtain the Game object from the parental hierarchy of an object
 * @param gameObject A QObject associated with the desired Game
 * @return The Game if found, otherwise 0
 */
extern Game* getGame( QObject* gameObject );

#endif // GAMEUTILS_H

#ifndef LOADABLE_H
#define LOADABLE_H

/**
 * @brief Interface used for loading recording data
 */
class Loadable
{
public:
    virtual ~Loadable() = default;

    virtual char* getLoadableDestination( int forLevel, int count ) = 0;
    virtual void releaseLoadableDestination( int forLevel, int actualCount ) = 0;
};

#endif // LOADABLE_H

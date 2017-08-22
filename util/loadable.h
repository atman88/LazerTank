#ifndef LOADABLE_H
#define LOADABLE_H

class Loadable
{
public:
    virtual ~Loadable() {}

    virtual char* getLoadableDestination( int forLevel, int count ) = 0;
    virtual void releaseLoadableDestination( int forLevel, int actualCount ) = 0;
};

#endif // LOADABLE_H

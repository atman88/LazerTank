#include <algorithm>
#include <QDir>

#include "level.h"
#include "controller/gameregistry.h"

Level::Level(int number) : mNumber(number)
{
}

bool Level::operator==( const Level& other ) const
{
    return mNumber == other.mNumber;
}

bool Level::operator<( const Level& other ) const
{
    return mNumber < other.mNumber;
}

int Level::getNumber() const
{
    return mNumber;
}

uint qHash( const Level& level )
{
    return level.getNumber();
}

LevelList::LevelList() : QObject(0)
{
}

class DirLoadRunnable : public Runnable
{
public:
    DirLoadRunnable( LevelList* list ) : Runnable(true), mList(list)
    {
    }

    ~DirLoadRunnable()
    {
    }

    void run() override
    {
        QString pattern( "level*.txt" );
        int numberStartOffset = pattern.indexOf( QChar('*') );
        QDir dir( ":/maps", pattern, QDir::Unsorted, QDir::Files|QDir::NoDotAndDotDot|QDir::Readable );
        QStringList entries = dir.entryList();
        QList<Level>& list = mList->mList;
        for( QString entry : entries ) {
            bool ok;
            int number = entry.mid( numberStartOffset, entry.length()-numberStartOffset-4 ).toInt( &ok );
            if ( ok ) {
                list.append(Level(number));
            }
        }
        std::sort( list.begin(), list.end() );

        emit mList->initialized();
    }

private:
    LevelList* mList;
};

void LevelList::init( GameRegistry* registry )
{
    registry->getWorker().doWork( new DirLoadRunnable( this ) );
}

int LevelList::nextLevel( int curLevel ) const
{
    int index;
    if ( curLevel < 0 ) {
        index = 0;
    } else {
        if ( (index = mList.indexOf( Level(curLevel) )) < 0 ) {
            return 0;
        }
        ++index;
    }
    if ( index < mList.size() ) {
        return mList.at(index).getNumber();
    }
    return 0;
}

QList<Level> LevelList::getList() const
{
    return mList;
}

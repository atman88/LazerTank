#include <iostream>
#include <set>
#include "../testmain.h"
#include "model/boardpool.h"
#include "controller/gameregistry.h"

using namespace std;

class TestPool : public BoardPool
{
public:
    TestPool( int visibleCount, int size ) : BoardPool(visibleCount, size)
    {
    }

    void testLoadNew( int level, int expectedFirstVisible = 0 )
    {
        cout << "testLoadNew(" << level << "," << expectedFirstVisible << ")" << endl;

        QSignalSpy loadSpy( this, &BoardPool::boardLoaded );
        QVERIFY( !getBoard(level) );
        if ( expectedFirstVisible ) {
            QVERIFY( mFirstVisible == expectedFirstVisible );
        }
        QVERIFY( loadSpy.wait( 1000 ) );
    }

    void printContents()
    {
        bool separate = false;
        cout << "contents: ";
        for( auto it : mPool ) {
            if ( separate ) {
                cout << ",";
            } else {
                separate = true;
            }
            cout << it.second->getLevel();
        }
        cout << endl;
    }

    void verifyContents( std::set<int> levels )
    {
        int i;
        int max = (*levels.cend()) + 1;
        auto it = levels.cbegin();
        if ( it == levels.cend() ) {
            i = max = 0;
        } else {
            i = (*it) - 1;
            max = (*--(levels.cend())) + 1;
        }
        for( ; i <= max; ++i ) {
            if ( i != *it ) {
                // confirm hole:
                if ( find(i) ) {
                    printContents();
                    QFAIL( qPrintable(QString("find(%1)==0").arg( i )) );
                }
            } else {
                // confirm element:
                if ( find(i)->getLevel() != i ) {
                    printContents();
                    QFAIL( qPrintable(QString("find(%1)->getLevel()").arg( i )) );
                }
                ++it;
            }
        }
    }
};

void TestMain::testBoardPool()
{
    TestPool* testPool = new TestPool( 2, 3 );
    mRegistry.injectBoardPool( testPool );

    QVERIFY( !testPool->find(10) );

    testPool->testLoadNew( 10, 10 );
    if ( Board* board = testPool->getBoard(10) ) {
        QVERIFY( board->getLevel() == 10 );
    } else {
        QFAIL( "getBoard returned null" );
    }

    testPool->testLoadNew( 11, 10 );
    testPool->testLoadNew(  9,  9 );
    testPool->verifyContents( { 9, 10, 11 } );

    // now that it's at capacity, test recycling:
    testPool->testLoadNew( 12, 11 );

    testPool->verifyContents( { 10, 11, 12 } );
}

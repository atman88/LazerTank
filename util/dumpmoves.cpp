#include <cstdio>

#include "util/gameutils.h"

void dumpMoves( const char* name, EncodedMove* moves, int count, int chevronPos )
{
    printf( "%s %d:\n", name, count );
    for( int i = 0; i < count; ++i ) {
        EncodedMove move = moves[i];

         if ( move.isEmpty() ) {
             puts( "*empty*" );
             break;
         }

         if ( move.u.continuation.header == 0 )
             printf( "contine %d\n", move.u.continuation.shotCount );
         else {
             int somethingWritten = 0;
             if ( move.u.move.adjacent != 0 ) somethingWritten = fputs( "adjacent", stdout );
             if ( move.u.move.rotate != 0 ) {
                 if ( somethingWritten != 0 ) {
                     putc( ',', stdout );
                 }
                 switch( move.u.move.encodedAngle ) {
                 case 0: fputs( "up",   stdout ); break;
                 case 1: fputs( "right",stdout ); break;
                 case 2: fputs( "down", stdout ); break;
                 case 3: fputs( "left", stdout ); break;
                 }
                 somethingWritten = 1;
             }
             if ( move.u.move.shotCount ) {
                 if ( somethingWritten != 0 ) {
                     putc( ',', stdout );
                 }
                 printf( "%d", move.u.move.shotCount );
             }
             if ( i == chevronPos ) {
                 fputs( "\t<<<", stdout );
             }
             putc( '\n', stdout );
         }
    }
    fflush(stdout);
}

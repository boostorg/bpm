//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "options.hpp"

void parse_options( char const * * & argv, void (*handle_option)( std::string const & opt ) )
{
    while( argv[ 0 ] && ( argv[ 0 ][ 0 ] == '-' || argv[ 0 ][ 0 ] == '+' ) )
    {
        if( argv[ 0 ][ 0 ] == '-' && argv[ 0 ][ 1 ] == '-' )
        {
            if( argv[ 0 ][ 2 ] == 0 )
            {
                // -- turns off option processing

                ++argv;
                break;
            }
            else
            {
                handle_option( argv[ 0 ] );
            }
        }
        else
        {
            for( char const * p = argv[ 0 ] + 1; *p; ++p )
            {
                char opt[ 3 ] = { argv[ 0 ][ 0 ], *p, 0 };
                handle_option( opt );
            }
        }

        ++argv;
    }
}

//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "message.hpp"
#include <stdio.h>
#include <stdarg.h>

static int s_level;

int get_message_level()
{
    return s_level;
}

void set_message_level( int level )
{
    s_level = level;
}

void increase_message_level()
{
    ++s_level;
}

void decrease_message_level()
{
    --s_level;
}

void msg_printf( int level, char const * format, ... )
{
    if( level <= s_level )
    {
        fprintf( stderr, "bpm: " );

        va_list args;
        va_start( args, format );

        vfprintf( stderr, format, args );

        va_end( args );

        fprintf( stderr, "\n" );
    }
}

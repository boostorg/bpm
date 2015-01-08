//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "cmd_list.hpp"
#include "options.hpp"
#include "dependencies.hpp"
#include "message.hpp"
#include "fs.hpp"
#include <algorithm>
#include <stdexcept>
#include <cstdio>

static bool s_opt_a = false;
static bool s_opt_i = false;
static bool s_opt_p = false;
static bool s_opt_b = false;

static void handle_option( std::string const & opt )
{
    if( opt == "-a" )
    {
        s_opt_a = true;
    }
    else if( opt == "-i" )
    {
        s_opt_i = true;
    }
    else if( opt == "-p" )
    {
        s_opt_p = true;
    }
    else if( opt == "-b" )
    {
        s_opt_b = true;
    }
    else if( opt == "-v" )
    {
        increase_message_level();
    }
    else if( opt == "-q" )
    {
        decrease_message_level();
    }
    else
    {
        throw std::runtime_error( "invalid list option: '" + opt + "'" );
    }
}

static std::string module_package( std::string const & module )
{
    std::size_t i = module.find( '~' );

    std::string package = module.substr( 0, i );

    return package;
}

void cmd_list( char const * argv[] )
{
    parse_options( argv, handle_option );

    std::string prefix;

    if( *argv )
    {
        prefix = *argv++;
    }

    if( *argv )
    {
        throw std::runtime_error( std::string( "unexpected list argument '" ) + *argv + "'" );
    }

    if( s_opt_a + s_opt_i + s_opt_p > 1 )
    {
        throw std::runtime_error( "list options -a, -i and -p are mutually exclusive" );
    }

    if( s_opt_p && s_opt_b )
    {
        throw std::runtime_error( "list options -p and -b are incompatible" );
    }

    if( !s_opt_a && !s_opt_i && !s_opt_p )
    {
        s_opt_a = !s_opt_b;
        s_opt_i = s_opt_b;
    }

    std::map< std::string, std::vector< std::string > > deps;
    std::set< std::string > buildable;

    retrieve_dependencies( deps, buildable );

    if( s_opt_a )
    {
        for( std::map< std::string, std::vector< std::string > >::const_iterator i = deps.begin(); i != deps.end(); ++i )
        {
            std::string module = i->first;

            if( module.substr( 0, prefix.size() ) != prefix ) continue;

            if( !s_opt_b || buildable.count( module ) )
            {
                printf( "%s\n", module.c_str() );
            }
        }
    }
    else
    {
        for( std::map< std::string, std::vector< std::string > >::const_iterator i = deps.begin(); i != deps.end(); ++i )
        {
            std::string module = i->first;

            if( module.substr( 0, prefix.size() ) != prefix ) continue;

            std::string package = module_package( module );

            std::string path( module );
            std::replace( path.begin(), path.end(), '~', '/' );

            if( fs_exists( "libs/" + path ) && s_opt_p != fs_exists( "libs/" + package + "/.installed" ) )
            {
                if( !s_opt_b || buildable.count( module ) )
                {
                    printf( "%s\n", module.c_str() );
                }
            }
        }
    }
}

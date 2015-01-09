//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "cmd_remove.hpp"
#include "cmd_headers.hpp"
#include "cmd_index.hpp"
#include "options.hpp"
#include "dependencies.hpp"
#include "message.hpp"
#include "fs.hpp"
#include <algorithm>
#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <errno.h>

static bool s_opt_n = false;
static bool s_opt_f = false;
static bool s_opt_d = false;
static bool s_opt_a = false;
static bool s_opt_p = false;

static void handle_option( std::string const & opt )
{
    if( opt == "-n" )
    {
        s_opt_n = true;
    }
    else if( opt == "-f" )
    {
        s_opt_f = true;
    }
    else if( opt == "-d" )
    {
        s_opt_d = true;
    }
    else if( opt == "-a" )
    {
        s_opt_a = true;
    }
    else if( opt == "-p" )
    {
        s_opt_p = true;
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
        throw std::runtime_error( "invalid remove option: '" + opt + "'" );
    }
}

static std::string module_package( std::string const & module )
{
    std::size_t i = module.find( '~' );

    std::string package = module.substr( 0, i );

    return package;
}

static void get_package_dependents( std::string const & package, std::map< std::string, std::vector< std::string > > const & deps, std::vector< std::string > const & packages, std::set< std::string > & deps2 )
{
    deps2.clear();

    if( !fs_exists( "libs/" + package + "/.installed" ) )
    {
        // partially installed packages have no dependents
        return;
    }

    for( std::map< std::string, std::vector< std::string > >::const_iterator i = deps.begin(); i != deps.end(); ++i )
    {
        std::string m1 = i->first;
        std::string p1 = module_package( m1 );

        for( std::vector< std::string >::const_iterator j = i->second.begin(); j != i->second.end(); ++j )
        {
            std::string m2 = *j;
            std::string p2 = module_package( m2 );

            // p1 -> p2

            if( p2 == package && fs_exists( "libs/" + p1 + "/.installed" ) && std::find( packages.begin(), packages.end(), p1 ) == packages.end() )
            {
                deps2.insert( p1 );
            }
        }
    }
}

static void removing( std::string const & path )
{
    msg_printf( 2, "removing '%s'", path.c_str() );
}

static void rmerror( std::string const & path, int err )
{
    msg_printf( 1, "'%s': remove error: %s", path.c_str(), std::strerror( err ) );
}

static void remove_package( std::string const & package, int & removed )
{
    std::string path = "libs/" + package;

    std::set< std::string > files;

    if( package == "build" )
    {
        path = "tools/" + package;

        files.insert( "b2.exe" );
        files.insert( "boost-build.jam" );
        files.insert( "boostcpp.jam" );
        files.insert( "Jamroot" );
        files.insert( "libs/Jamfile.v2" );
    }

    if( !fs_exists( path ) )
    {
        msg_printf( 1, "package '%s' has already been removed", package.c_str() );
        return;
    }

    if( s_opt_n )
    {
        msg_printf( 0, "would have removed package '%s'", package.c_str() );
        return;
    }

    msg_printf( 0, "removing package '%s'", package.c_str() );

    std::string marker = path + "/.installed";

    std::remove( marker.c_str() );

    fs_remove_all( path, removing, rmerror );

    ++removed;

    for( std::set< std::string >::const_iterator i = files.begin(); i != files.end(); ++i )
    {
        if( fs_exists( *i ) )
        {
            removing( *i );

            int r = std::remove( i->c_str() );

            if( r != 0 )
            {
                rmerror( *i, errno );
            }
        }
    }
}

void cmd_remove( char const * argv[] )
{
    parse_options( argv, handle_option );

    if( s_opt_a && s_opt_p )
    {
        throw std::runtime_error( "remove options -a and -p are mutually exclusive" );
    }

    if( ( s_opt_a || s_opt_p ) && *argv != 0 )
    {
        throw std::runtime_error( "remove options -a and -p are incompatible with a package list" );
    }

    if( s_opt_d && !s_opt_f && !s_opt_n )
    {
        throw std::runtime_error( "remove option -d requires -f" );
    }

    std::map< std::string, std::vector< std::string > > deps;
    std::set< std::string > buildable;

    retrieve_dependencies( deps, buildable );

    std::vector< std::string > packages;

    if( s_opt_a )
    {
        if( !s_opt_f && !s_opt_n )
        {
            throw std::runtime_error( "remove option -a requires -f" );
        }

        for( std::map< std::string, std::vector< std::string > >::const_iterator i = deps.begin(); i != deps.end(); ++i )
        {
            std::string module = i->first;
            std::string package = module_package( module );

            if( packages.empty() || packages.back() != package )
            {
                packages.push_back( package );
            }
        }

        packages.push_back( "build" );
    }
    else if( s_opt_p )
    {
        for( std::map< std::string, std::vector< std::string > >::const_iterator i = deps.begin(); i != deps.end(); ++i )
        {
            std::string module = i->first;
            std::string package = module_package( module );

            if( fs_exists( "libs/" + package ) && !fs_exists( "libs/" + package + "/.installed" ) )
            {
                if( packages.empty() || packages.back() != package )
                {
                    packages.push_back( package );
                }
            }

            if( fs_exists( "tools/build" ) && !fs_exists( "tools/build/.installed" ) )
            {
                packages.push_back( "build" );
            }
        }
    }
    else
    {
        while( *argv )
        {
            packages.push_back( *argv );
            ++argv;
        }
    }

    if( !s_opt_f && !( s_opt_n && s_opt_d ) )
    {
        for( std::vector< std::string >::const_iterator i = packages.begin(); i != packages.end(); ++i )
        {
            std::string package = *i;

            std::set< std::string > deps2;

            get_package_dependents( package, deps, packages, deps2 );

            if( !s_opt_f && !deps2.empty() )
            {
                std::string list;

                for( std::set< std::string >::const_iterator j = deps2.begin(); j != deps2.end(); ++j )
                {
                    list += " ";
                    list += *j;
                }

                msg_printf( -2, "package '%s' cannot be removed due to dependents:\n %s", package.c_str(), list.c_str() );

                return;
            }
        }
    }

    int removed = 0;

    {
        std::size_t i = 0;

        while( i < packages.size() )
        {
            std::string package = packages[ i++ ];

            std::set< std::string > deps2;

            get_package_dependents( package, deps, packages, deps2 );

            remove_package( package, removed );

            if( s_opt_d )
            {
                for( std::set< std::string >::const_iterator j = deps2.begin(); j != deps2.end(); ++j )
                {
                    if( std::find( packages.begin(), packages.end(), *j ) == packages.end() )
                    {
                        packages.push_back( *j );
                    }
                }
            }
        }
    }

    if( removed )
    {
        cmd_headers();
    }

    if( removed )
    {
        cmd_index();
    }
}

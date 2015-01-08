//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "cmd_headers.hpp"
#include "options.hpp"
#include "message.hpp"
#include "error.hpp"
#include "fs.hpp"
#include <stdexcept>
#include <map>
#include <cassert>
#include <errno.h>

static void handle_option( std::string const & opt )
{
    if( opt == "-v" )
    {
        increase_message_level();
    }
    else if( opt == "-q" )
    {
        decrease_message_level();
    }
    else
    {
        throw std::runtime_error( "invalid headers option: '" + opt + "'" );
    }
}

void cmd_headers( char const * argv[] )
{
    parse_options( argv, handle_option );

    if( *argv )
    {
        throw std::runtime_error( std::string( "unexpected argument '" ) + *argv + "'" );
    }

    cmd_headers();
}

static void removing( std::string const & path )
{
    msg_printf( 2, "removing '%s'", path.c_str() );
}

static void rmerror( std::string const & path, int err )
{
    throw_errno_error( path, "remove error", err );
}

static void build_dir_map2( std::string const & path, std::string const & suffix, std::map< std::string, std::vector< std::string > > & dirs )
{
    // enumerate header directories in 'path' + 'suffix'

    std::vector< std::string > entries;
    int r = fs_readdir( path + suffix, entries );

    if( r != 0 )
    {
        throw_errno_error( path + suffix, "read error", errno );
    }

    for( std::vector< std::string >::const_iterator i = entries.begin(); i != entries.end(); ++i )
    {
        if( *i == "." || *i == ".." ) continue;

        std::string sx2 = suffix + "/" + *i;

        std::string p2 = path + sx2;

        if( !fs_is_dir( p2 ) ) continue;

        dirs[ sx2 ].push_back( p2 );

        build_dir_map2( path, sx2, dirs );
    }
}

static void build_dir_map( std::string const & path, std::map< std::string, std::vector< std::string > > & dirs )
{
    // enumerate modules in 'path'

    std::vector< std::string > entries;
    int r = fs_readdir( path, entries );

    if( r != 0 )
    {
        throw_errno_error( path, "read error", errno );
    }

    for( std::vector< std::string >::const_iterator i = entries.begin(); i != entries.end(); ++i )
    {
        if( *i == "." || *i == ".." ) continue;

        std::string p2 = path + "/" + *i;

        if( fs_is_dir( p2 ) && fs_is_dir( p2 + "/include" ) )
        {
            build_dir_map2( p2 + "/", "include", dirs );
        }

        if( fs_exists( p2 + "/sublibs" ) )
        {
            // enumerate submodules
            build_dir_map( p2, dirs );
        }
    }
}

static void link_directory( std::string const & dir, std::map< std::string, std::vector< std::string > > & dirs );

static void link_files( std::string const & path, std::string const & target, std::map< std::string, std::vector< std::string > > & dirs )
{
    // msg_printf( 1, "linking files from '%s' into '%s'", path.c_str(), target.c_str() );

    std::vector< std::string > entries;
    int r = fs_readdir( path, entries );

    if( r != 0 )
    {
        throw_errno_error( path, "read error", errno );
    }

    for( std::vector< std::string >::const_iterator i = entries.begin(); i != entries.end(); ++i )
    {
        if( *i == "." || *i == ".." ) continue;

        std::string p2 = path + "/" + *i;
        std::string t2 = target + "/" + *i;

        if( fs_is_dir( p2 ) )
        {
            link_directory( t2, dirs );
        }
        else
        {
            // file
            msg_printf( 1, "linking '%s' to '%s'", t2.c_str(), p2.c_str() );

            if( fs_link_file( t2, p2 ) != 0 )
            {
                throw_errno_error( t2, "link create error", errno );
            }
        }
    }
}

static void remove_directory_from_list( std::string const & dir, std::map< std::string, std::vector< std::string > > & dirs )
{
    for( std::map< std::string, std::vector< std::string > >::iterator i = dirs.find( dir ); i != dirs.end(); )
    {
        std::string d2 = i->first;

        if( d2 == dir || d2.substr( 0, 1 + dir.size() ) == dir + "/" )
        {
            dirs.erase( i++ );
        }
        else
        {
            break;
        }
    }
}

static void link_directory( std::string const & dir, std::string const & target )
{
    msg_printf( 1, "linking '%s' to '%s'", dir.c_str(), target.c_str() );

    if( fs_link_dir( dir, target ) != 0 )
    {
        throw_errno_error( dir, "link create error", errno );
    }
}

static void create_directory( std::string const & dir )
{
    msg_printf( 1, "creating '%s'", dir.c_str() );

    if( fs_mkdir( dir, 0755 ) != 0 )
    {
        throw_errno_error( dir, "create error", errno );
    }
}

static void link_directory( std::string const & dir, std::map< std::string, std::vector< std::string > > & dirs )
{
    if( dirs.count( dir ) == 0 )
    {
        // already processed
        return;
    }

    std::vector< std::string > const & d2 = dirs[ dir ];

    assert( !d2.empty() );

    if( d2.size() == 1 )
    {
        link_directory( dir, d2.front() );
    }
    else
    {
        create_directory( dir );

        for( std::vector< std::string >::const_iterator i = d2.begin(); i != d2.end(); ++i )
        {
            link_files( *i, dir, dirs );
        }
    }

    remove_directory_from_list( dir, dirs );
}

static void remove_directory( std::string const & dir )
{
    if( fs_exists( dir ) )
    {
        fs_remove_all( dir, removing, rmerror );
    }
}

static void touch_file( std::string const & path )
{
    int fd = fs_creat( path, 0644 );

    if( fd >= 0 )
    {
        fs_close( fd );
    }
}

void cmd_headers()
{
    msg_printf( 0, "recreating header links" );

    msg_printf( 1, "removing old header links" );

    remove_directory( "include" );
    remove_directory( "boost" );

    if( !fs_exists( "libs" ) )
    {
        return;
    }

    std::map< std::string, std::vector< std::string > > dirs;
    build_dir_map( "libs", dirs );

    if( dirs.empty() )
    {
        return;
    }

    create_directory( "include" );

    while( !dirs.empty() )
    {
        std::string dir = dirs.begin()->first;
        link_directory( dir, dirs );
    }

    touch_file( "include/.updated" );

    if( fs_exists( "include/boost" ) )
    {
        link_directory( "boost", "include/boost" );
    }
}

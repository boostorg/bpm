//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "options.hpp"
#include "dependencies.hpp"
#include "message.hpp"
#include "package_path.hpp"
#include "cmd_headers.hpp"
#include "cmd_index.hpp"

#include "lzma_reader.hpp"
#include "http_reader.hpp"
#include "tar.hpp"

#include "error.hpp"
#include "fs.hpp"

#include <algorithm>
#include <stdexcept>
#include <cstdio>
#include <cstddef>
#include <cstring>
#include <errno.h>


static bool s_opt_n = false;
static bool s_opt_d = true;
static bool s_opt_k = false;
static bool s_opt_a = false;
static bool s_opt_i = false;
static bool s_opt_p = false;

static void handle_option( std::string const & opt )
{
    if( opt == "-n" )
    {
        s_opt_n = true;
    }
    else if( opt == "+d" )
    {
        s_opt_d = false;
    }
    else if( opt == "-d" )
    {
        s_opt_d = true;
    }
    else if( opt == "-k" )
    {
        s_opt_k = true;
    }
    else if( opt == "-a" )
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
        throw std::runtime_error( "invalid install option: '" + opt + "'" );
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

static void remove_partial_installation( std::string const & module, std::string const & path, std::set< std::string > const & files )
{
    if( fs_exists( path ) )
    {
        msg_printf( 1, "removing partial installation of module '%s'", module.c_str() );
        fs_remove_all( path, removing, rmerror );
    }

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

static void touch_file( std::string const & path )
{
    int fd = fs_creat( path, 0644 );

    if( fd >= 0 )
    {
        fs_close( fd );
    }
}

static std::string module_package( std::string const & module )
{
    std::size_t i = module.find( '~' );

    std::string package = module.substr( 0, i );

    return package;
}

static void install_module( std::string const & package_path, std::string const & module, std::set< std::string > & installed, std::time_t & mtime )
{
    std::string package = module_package( module );

    std::string path = "libs/" + package;

    std::set< std::string > whitelist;

    if( module == "build" )
    {
        if( !fs_exists( "tools" ) )
        {
            fs_mkdir( "tools/", 0755 );
        }

        path = "tools/" + module;

        whitelist.insert( "b2.exe" );
        whitelist.insert( "boost-build.jam" );
        whitelist.insert( "boostcpp.jam" );
        whitelist.insert( "Jamroot" );
        whitelist.insert( "libs/Jamfile.v2" );
    }

    std::string marker = path + "/.installed";

    if( !fs_exists( marker ) )
    {
        if( s_opt_n )
        {
            msg_printf( 0, "would have installed module '%s'", module.c_str() );
        }
        else
        {
            remove_partial_installation( module, path, whitelist );

            msg_printf( 0, "installing module '%s'", module.c_str() );

            std::string tar_path = package_path + package + ".tar.lzma";

            http_reader r1( tar_path );
            lzma_reader r2( &r1 );

            try
            {
                tar_extract( &r2, path + '/', whitelist );
                touch_file( marker );
            }
            catch( std::exception const & )
            {
                if( !s_opt_k )
                {
                    remove_partial_installation( module, path, whitelist );
                }

                throw;
            }
        }

        installed.insert( module );
    }
    else
    {
        msg_printf( 1, "module '%s' is already installed", module.c_str() );
    }

    mtime = std::max( mtime, fs_mtime( marker ) );
}

static void install_boost_build( std::string const & package_path, std::set< std::string > & installed )
{
    std::time_t mtime = 0;
    install_module( package_path, "build", installed, mtime );
}

void cmd_install( char const * argv[] )
{
    parse_options( argv, handle_option );

    if( s_opt_a + s_opt_i + s_opt_p > 1 )
    {
        throw std::runtime_error( "install options -a, -i and -p are mutually exclusive" );
    }

    if( ( s_opt_a || s_opt_i || s_opt_p ) && *argv != 0 )
    {
        throw std::runtime_error( "install options -a, -i and -p are incompatible with a module list" );
    }

    std::string package_path = get_package_path();

    std::map< std::string, std::vector< std::string > > deps;
    std::set< std::string > buildable;

    retrieve_dependencies( deps, buildable );

    if( !fs_exists( "libs" ) )
    {
        fs_mkdir( "libs/", 0755 );
    }

    std::vector< std::string > modules;

    if( s_opt_a )
    {
        for( std::map< std::string, std::vector< std::string > >::const_iterator i = deps.begin(); i != deps.end(); ++i )
        {
            modules.push_back( i->first );
        }
    }
    else if( s_opt_i || s_opt_p )
    {
        for( std::map< std::string, std::vector< std::string > >::const_iterator i = deps.begin(); i != deps.end(); ++i )
        {
            std::string module = i->first;

            std::string package = module_package( module );

            std::string path( module );
            std::replace( path.begin(), path.end(), '~', '/' );

            if( fs_exists( "libs/" + path ) && s_opt_p != fs_exists( "libs/" + package + "/.installed" ) )
            {
                modules.push_back( i->first );
            }
        }
    }
    else
    {
        while( *argv )
        {
            modules.push_back( *argv );
            ++argv;
        }
    }

    std::set< std::string > installed;

    std::time_t mtime = 0;

    {
        std::size_t i = 0;

        while( i < modules.size() )
        {
            std::string module = modules[ i++ ];

            if( deps.count( module ) == 0 )
            {
                msg_printf( -1, "module '%s' does not exist", module.c_str() );
            }
            else
            {
                install_module( package_path, module, installed, mtime );

                if( s_opt_d )
                {
                    std::vector< std::string > mdeps = deps[ module ];

                    for( std::size_t j = 0, n = mdeps.size(); j < n; ++j )
                    {
                        std::string const & m2 = mdeps[ j ];

                        if( std::find( modules.begin(), modules.end(), m2 ) == modules.end() )
                        {
                            modules.push_back( m2 );
                        }
                    }
                }
            }
        }
    }

    std::set< std::string > installed2;

    {
        bool need_build = false;

        for( std::vector< std::string >::const_iterator i = modules.begin(); i != modules.end(); ++i )
        {
            if( buildable.count( *i ) )
            {
                need_build = true;
            }
        }

        if( s_opt_d && need_build )
        {
            install_boost_build( package_path, installed2 );
        }
    }

    if( installed.empty() && installed2.empty() )
    {
        msg_printf( 0, "nothing to install, everything is already in place" );
    }

    if( !s_opt_n )
    {
        std::set< std::string > need_build;

        for( std::set< std::string >::const_iterator i = installed.begin(); i != installed.end(); ++i )
        {
            if( buildable.count( *i ) )
            {
                need_build.insert( *i );
            }
        }

        if( !need_build.empty() )
        {
            std::string m2;

            for( std::set< std::string >::const_iterator i = need_build.begin(); i != need_build.end(); ++i )
            {
                m2 += " ";
                m2 += *i;
            }

            msg_printf( 0, "the following libraries need to be built:\n %s", m2.c_str() );
#if defined( _WIN32 )
            msg_printf( 0, "(use b2 to build)" );
#else
            msg_printf( 0, "(use ./b2 to build)" );
#endif
        }
    }

    if( !s_opt_n && mtime >= fs_mtime( "include/.updated" ) ) // headers out of date
    {
        cmd_headers();
    }

    if( !s_opt_n && mtime >= fs_mtime( "index.html" ) ) // index out of date
    {
        cmd_index();
    }
}

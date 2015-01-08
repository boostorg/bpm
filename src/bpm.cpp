//
// bpm - a tool to install Boost modules
//
// Copyright 2014, 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "message.hpp"
#include "error.hpp"
#include "config.hpp"
#include "options.hpp"
#include "cmd_install.hpp"
#include "cmd_headers.hpp"
#include "cmd_index.hpp"
#include "cmd_remove.hpp"
#include "cmd_list.hpp"
#include <string>
#include <exception>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>

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
        throw std::runtime_error( "invalid option: '" + opt + "'" );
    }
}

static void usage()
{
    printf( "%s",

        "Usage: bpm [-v] [-q] command [options] [modules]\n\n"

        "  -v:  Be verbose\n"
        "  -vv: Be more verbose\n"
        "  -q:  Be quiet\n\n"

        "  bpm install [-n] [+d] [-k] [-a] [-i] [-p] <module> <module>...\n\n"

        "    Installs the specified modules and their dependencies into\n"
        "    the current directory.\n\n"

        "    -n: Only output what would be installed\n"
        "    +d: Do not install dependencies\n"
        "    -k: Do not remove partial installations on error\n"
        "    -a: All modules (use instead of a module list)\n"
        "    -i: Installed modules\n"
        "    -p: Partially installed modules\n\n"

        "  bpm remove [-n] [-f] [-d] [-a] [-p] <package> <package>...\n\n"

        "    Removes the specified packages.\n\n"

        "    (A package, like 'numeric', can contain more than one module.)\n\n"

        "    -n: Only output what would be removed\n"
        "    -f: Force removal even when dependents exist\n"
        "    -d: Remove dependents as well. Requires -f\n"
        "    -a: Remove all packages. Requires -f\n"
        "    -p: Remove partially installed packages\n\n"

        "  bpm list [-a] [-i] [-p] [-b] [prefix]\n\n"

        "    Lists modules matching [prefix].\n\n"

        "    -a: All modules (default when no -b)\n"
        "    -i: Installed modules (default when -b)\n"
        "    -p: Partially installed modules\n"
        "    -b: Modules that require building\n\n"

        "  bpm headers\n\n"

        "    Recreates the header links in the include/ subdirectory of\n"
        "    the current directory.\n\n"

        "  bpm index\n\n"

        "    Recreates the file index.html, which lists the installed\n"
        "    modules, in the current directory.\n"

    );
}

int main( int argc, char const * argv[] )
{
    if( argc < 2 || std::string( argv[ 1 ] ) == "--help" )
    {
        usage();
        return 0;
    }

    try
    {
        ++argv;

        config_read_file( "bpm.conf" );

        parse_options( argv, handle_option );

        if( *argv == 0 )
        {
            throw std::runtime_error( "missing command" );
        }

        std::string command( *argv++ );

        if( command == "install" )
        {
            cmd_install( argv );
        }
        else if( command == "remove" )
        {
            cmd_remove( argv );
        }
        else if( command == "headers" )
        {
            cmd_headers( argv );
        }
        else if( command == "index" )
        {
            cmd_index( argv );
        }
        else if( command == "list" )
        {
            cmd_list( argv );
        }
        else if( command == "help" )
        {
            usage();
        }
        else
        {
            throw std::runtime_error( "invalid command: '" + command + "'" );
        }
    }
    catch( std::exception const & x )
    {
        fprintf( stderr, "bpm: %s\n", x.what() );
        return EXIT_FAILURE;
    }
}

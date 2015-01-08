//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "dependencies.hpp"
#include "package_path.hpp"
#include "lzma_reader.hpp"
#include "http_reader.hpp"
#include "error.hpp"
#include "string.hpp"
#include <stdexcept>
#include <sstream>

static void parse_module_description( std::string const & name, std::string const & line, std::map< std::string, std::vector< std::string > > & deps )
{
    std::istringstream is( line );

    std::string module, arrow;

    if( is >> module >> arrow && arrow == "->" )
    {
        deps[ module ];

        std::string dep;

        while( is >> dep )
        {
            deps[ module ].push_back( dep );
        }
    }
    else
    {
        throw_error( name, "invalid line: '" + line + "'" );
    }
}

static std::string read_text_file( std::string const & url )
{
    http_reader r1( url );
    lzma_reader r2( &r1 );

    std::string data;

    for( ;; )
    {
        int const N = 4096;

        char buffer[ N ];

        std::size_t r = r2.read( buffer, N );

        data.append( buffer, r );

        if( r < N ) break;
    }

    return data;
}

static void retrieve_dependencies( std::string const & package_path, std::map< std::string, std::vector< std::string > > & deps )
{
    std::string url = package_path + "dependencies.txt.lzma";

    std::string data = read_text_file( url );

    std::istringstream is( data );

    std::string line;

    while( std::getline( is, line ) )
    {
        remove_trailing( line, '\r' );
        parse_module_description( url, line, deps );
    }
}

static void retrieve_buildable( std::string const & package_path, std::set< std::string > & buildable )
{
    std::string data = read_text_file( package_path + "buildable.txt.lzma" );

    std::istringstream is( data );

    std::string module;

    while( is >> module )
    {
        buildable.insert( module );
    }
}

void retrieve_dependencies( std::map< std::string, std::vector< std::string > > & deps, std::set< std::string > & buildable )
{
    deps.clear();
    buildable.clear();

    std::string package_path = get_package_path();

    retrieve_dependencies( package_path, deps );
    retrieve_buildable( package_path, buildable );
}

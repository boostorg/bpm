//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "config.hpp"
#include "error.hpp"
#include "string.hpp"
#include <map>
#include <fstream>
#include <cctype>
#include <cstddef>
#include <errno.h>

static std::map< std::string, std::string > s_config;

void config_read_file( std::string const & fn )
{
    std::ifstream is( fn.c_str() );

    if( !is )
    {
        throw_errno_error( fn, "open error", errno );
    }

    std::string line;

    while( std::getline( is, line ) )
    {
        if( line.empty() ) continue;

        if( !std::isalpha( static_cast< unsigned char >( line[0] ) ) ) continue;

        remove_trailing( line, '\r' );

        std::size_t i = line.find( '=' );

        if( i == 0 || i == std::string::npos ) continue;

        std::string key = line.substr( 0, i );
        std::string value = line.substr( i + 1 );

        s_config[ key ] = value;
    }
}

std::string config_get_option( std::string const & name )
{
    std::map< std::string, std::string >::const_iterator i = s_config.find( name );

    if( i == s_config.end() )
    {
        return std::string();
    }
    else
    {
        return i->second;
    }
}

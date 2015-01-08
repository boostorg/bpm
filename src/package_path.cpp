//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "package_path.hpp"
#include "config.hpp"
#include <stdexcept>

std::string get_package_path()
{
    std::string path = config_get_option( "package_path" );

    if( path.substr( 0, 7 ) != "http://" || *path.rbegin() != '/' )
    {
        throw std::runtime_error( "invalid package_path '" + path +  "' in bpm.conf" );
    }

    return path;
}

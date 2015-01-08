#ifndef CONFIG_HPP_INCLUDED
#define CONFIG_HPP_INCLUDED

//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include <string>

void config_read_file( std::string const & fn );
std::string config_get_option( std::string const & name );

#endif // #ifndef CONFIG_HPP_INCLUDED

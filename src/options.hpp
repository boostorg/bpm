#ifndef OPTIONS_HPP_INCLUDED
#define OPTIONS_HPP_INCLUDED

//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include <string>

void parse_options( char const * * & argv, void (*handle_option)( std::string const & opt ) );

#endif // #ifndef OPTIONS_HPP_INCLUDED

#ifndef JSON_HPP_INCLUDED
#define JSON_HPP_INCLUDED

//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include <string>
#include <vector>
#include <map>

void read_libraries_json( std::string const & name, std::vector< std::map< std::string, std::vector< std::string > > > & libraries );

#endif // #ifndef JSON_HPP_INCLUDED

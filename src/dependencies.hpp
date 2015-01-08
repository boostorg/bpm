#ifndef DEPENDENCIES_HPP_INCLUDED
#define DEPENDENCIES_HPP_INCLUDED

//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include <map>
#include <string>
#include <vector>
#include <set>

void retrieve_dependencies( std::map< std::string, std::vector< std::string > > & deps, std::set< std::string > & buildable );

#endif // #ifndef DEPENDENCIES_HPP_INCLUDED

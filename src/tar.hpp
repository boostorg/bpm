#ifndef TAR_HPP_INCLUDED
#define TAR_HPP_INCLUDED

//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "basic_reader.hpp"
#include <string>
#include <set>

void tar_extract( basic_reader * pr, std::string const & prefix, std::set< std::string > const & whitelist );

#endif // #ifndef TAR_HPP_INCLUDED

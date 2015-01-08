//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "string.hpp"

void remove_trailing( std::string & s, char ch )
{
    std::string::iterator last = s.end();

    if( last != s.begin() && ( --last, *last == ch ) )
    {
        s.erase( last );
    }
}

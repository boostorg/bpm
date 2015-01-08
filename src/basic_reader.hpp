#ifndef BASIC_READER_HPP_INCLUDED
#define BASIC_READER_HPP_INCLUDED

//
// Copyright 2014 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include <string>
#include <cstddef>

class basic_reader
{
protected:

    ~basic_reader()
    {
    }

public:

    virtual std::string name() const = 0;

    // read throws on error, and returns number of bytes read
    // which can be less than n when the end of the stream has
    // been reached

    virtual std::size_t read( void * p, std::size_t n ) = 0;
};

#endif // #ifndef BASIC_READER_HPP_INCLUDED

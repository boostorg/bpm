//
// Copyright 2014 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "file_reader.hpp"
#include "error.hpp"
#include <errno.h>

file_reader::file_reader( std::FILE * f, std::string const & name ): f_( f ), name_( name )
{
}

file_reader::file_reader( std::string const & fn ): name_( fn )
{
    f_ = std::fopen( fn.c_str(), "rb" );

    if( f_ == 0 )
    {
        throw_errno_error( fn, "open error", errno );
    }
}

file_reader::~file_reader()
{
    std::fclose( f_ );
}

std::string file_reader::name() const
{
    return name_;
}

std::size_t file_reader::read( void * p, std::size_t n )
{
    std::size_t r = std::fread( p, 1, n, f_ );

    if( r == 0 && std::ferror( f_ ) )
    {
        throw_errno_error( name(), "read error", errno );
    }

    return r;
}

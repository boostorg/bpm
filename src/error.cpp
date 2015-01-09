//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "error.hpp"
#include <cstring>

error::error( std::string const & nm, std::string const & rsn ): name_( nm ), reason_( rsn ), what_( "'" + nm + "': " + rsn )
{
}

error::~error() throw()
{
}

std::string error::name() const
{
    return name_;
}

std::string error::reason() const
{
    return reason_;
}

char const * error::what() const throw()
{
    return what_.c_str();
}

void throw_error( std::string const & name, std::string const & reason )
{
    throw error( name, reason );
}

void throw_errno_error( std::string const & name, std::string const & reason, int err )
{
    if( err == 0 )
    {
        throw_error( name, reason );
    }
    else
    {
        throw_error( name, reason + ": " + std::strerror( err ) );
    }
}

#if defined( _WIN32 )

#include <windows.h>

void throw_socket_error( std::string const & name, std::string const & reason, int err )
{
    char buffer[ 1024 ];

    if( err == 0 || FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0, err, 0, buffer, sizeof( buffer ), 0 ) == 0 )
    {
        throw_error( name, reason );
    }
    else
    {
        throw_error( name, reason + ": " + buffer );
    }
}

#else

void throw_socket_error( std::string const & name, std::string const & reason, int err )
{
    throw_errno_error( name, reason, err );
}

#endif

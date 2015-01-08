#ifndef ERROR_HPP_INCLUDED
#define ERROR_HPP_INCLUDED

//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include <string>
#include <exception>

class error: public std::exception
{
private:

    std::string name_;
    std::string reason_;

    std::string what_;

public:

    error( std::string const & name, std::string const & reason );
    ~error() throw();

    std::string name() const;
    std::string reason() const;

    char const * what() const throw();
};

void throw_error( std::string const & name, std::string const & reason );
void throw_errno_error( std::string const & name, std::string const & reason, int err );
void throw_socket_error( std::string const & name, std::string const & reason, int err );

#endif // #ifndef ERROR_HPP_INCLUDED

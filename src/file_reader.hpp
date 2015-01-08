#ifndef FILE_READER_HPP_INCLUDED
#define FILE_READER_HPP_INCLUDED

//
// Copyright 2014 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "basic_reader.hpp"
#include <cstdio>

class file_reader: public basic_reader
{
private:

    std::FILE * f_;
    std::string name_;

private:

    file_reader( file_reader const & );
    file_reader& operator=( file_reader const & );

public:

    explicit file_reader( std::FILE * f, std::string const & n );
    explicit file_reader( std::string const & fn );

    ~file_reader();

    virtual std::string name() const;
    virtual std::size_t read( void * p, std::size_t n );
};

#endif // #ifndef FILE_READER_HPP_INCLUDED

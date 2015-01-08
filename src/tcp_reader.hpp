#ifndef TCP_READER_HPP_INCLUDED
#define TCP_READER_HPP_INCLUDED

//
// Copyright 2014 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "basic_reader.hpp"

class tcp_reader: public basic_reader
{
private:

    std::ptrdiff_t sk_;
    std::string name_;

private:

    tcp_reader( tcp_reader const & );
    tcp_reader& operator=( tcp_reader const & );

public:

    explicit tcp_reader( std::string const & host, int port );

    ~tcp_reader();

    virtual std::string name() const;
    virtual std::size_t read( void * p, std::size_t n );

protected:

    void write( void const * p, std::size_t n );
};

#endif // #ifndef TCP_READER_HPP_INCLUDED

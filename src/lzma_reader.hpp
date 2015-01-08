#ifndef LZMA_READER_HPP_INCLUDED
#define LZMA_READER_HPP_INCLUDED

//
// Copyright 2014 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "basic_reader.hpp"

class lzma_reader: public basic_reader
{
private:

    basic_reader * pr_;

    unsigned char header_[ 13 ];
    unsigned char state_[ 128 ];

    static int const N = 4096;

    unsigned char buffer_[ N ];
    int k_, m_;

private:

    lzma_reader( lzma_reader const & );
    lzma_reader& operator=( lzma_reader const & );

public:

    explicit lzma_reader( basic_reader * pr );

    ~lzma_reader();

    virtual std::string name() const;
    virtual std::size_t read( void * p, std::size_t n );
};

#endif // #ifndef LZMA_READER_HPP_INCLUDED

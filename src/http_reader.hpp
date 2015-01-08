#ifndef HTTP_READER_HPP_INCLUDED
#define HTTP_READER_HPP_INCLUDED

//
// Copyright 2014 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "tcp_reader.hpp"

class http_url_parser
{
private:

    std::string host_;
    int port_;

    std::string request_;

public:

    explicit http_url_parser( std::string const & url );

    std::string host() const;
    int port() const;

    std::string request() const;
};

class http_reader: private http_url_parser, public tcp_reader
{
private:

    std::string name_;

private:

    http_reader( http_reader const & );
    http_reader& operator=( http_reader const & );

    std::string read_line();

public:

    explicit http_reader( std::string const & url );
    ~http_reader();

    virtual std::string name() const;
};

#endif // #ifndef HTTP_READER_HPP_INCLUDED

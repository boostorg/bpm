//
// Copyright 2014 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "http_reader.hpp"
#include "error.hpp"
#include <sstream>
#include <cstdlib>

http_url_parser::http_url_parser( std::string const & url )
{
    if( url.substr( 0, 7 ) != "http://" )
    {
        throw_error( url, "not an HTTP URL" );
    }

    std::size_t i = url.find( ':', 7 );
    std::size_t j = url.find( '/', 7 );

    if( j != std::string::npos )
    {
        request_ = url.substr( j );
    }
    else
    {
        request_ = "/";
    }

    if( i >= j )
    {
        // no port

        host_ = url.substr( 7, j - 7 );
        port_ = 80;
    }
    else
    {
        host_ = url.substr( 7, i - 7 );
        port_ = std::atoi( url.substr( i + 1, j - i - 1 ).c_str() );
    }
}

std::string http_url_parser::host() const
{
    return host_;
}

int http_url_parser::port() const
{
    return port_;
}

std::string http_url_parser::request() const
{
    return request_;
}

http_reader::http_reader( std::string const & url ): http_url_parser( url ), tcp_reader( this->host(), this->port() ), name_( url )
{
    std::string rq = "GET " + this->request() + " HTTP/1.0\r\nHost: " + this->host() + "\r\nConnection: close\r\n\r\n";

    this->write( rq.data(), rq.size() );

    std::string line = this->read_line(); // HTTP/1.0 (code) (message)

    {
        std::istringstream is( line );

        std::string http, message;
        int code;

        if( is >> http >> code >> message && http.substr( 0, 7 ) == "HTTP/1." && code >= 100 )
        {
            if( code >= 300 )
            {
                throw_error( name_, "HTTP error: " + line );
            }
        }
        else
        {
            throw_error( name_, "invalid server response: '" + line + "'" );
        }
    }

    // skip rest of header
    while( !read_line().empty() );
}

http_reader::~http_reader()
{
}

std::string http_reader::name() const
{
    return name_;
}

std::string http_reader::read_line()
{
    std::string r;

    for( ;; )
    {
        char ch;

        std::size_t r2 = read( &ch, 1 );

        if( r2 != 1 )
        {
            throw_error( name_, "unexpected end of data" );
        }

        if( ch == '\n' ) break;

        if( ch != '\r' )
        {
            r += ch;
        }
    }

    return r;
}

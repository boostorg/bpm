//
// Copyright 2014 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "tcp_reader.hpp"
#include "error.hpp"
#include <cassert>
#include <cstdio>

#ifdef _WIN32
# include <winsock.h>
#else

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>

typedef int SOCKET;

int const INVALID_SOCKET = -1;

inline int closesocket( SOCKET s )
{
    return close( s );
}

inline int WSAGetLastError()
{
    return errno;
}

#endif

static bool is_dotted_numeric( char const * p )
{
    for( ; *p; ++p )
    {
        if( !( *p >= '0' && *p <= '9' ) && *p != '.' )
        {
            return false;
        }
    }

    return true;
}

tcp_reader::tcp_reader( std::string const & host, int port )
{
    {
        char buffer[ 32 ];
        std::sprintf( buffer, "%d", port );

        name_ = host + ':' + buffer;
    }

    if( port <= 0 || port >= 65536 )
    {
        throw_error( name_, "invalid port number" );
    }

    unsigned long a;

    char const * p = host.c_str();

    if( is_dotted_numeric( p ) )
    {
        a = inet_addr( p );

        if( a == INADDR_NONE || a == 0 )
        {
            throw_error( host, "invalid host address" );
        }
    }
    else
    {
        hostent const * q = gethostbyname( p );

        if( q == 0 || q->h_length != 4 )
        {
            throw_error( host, "unable to resolve host name" );
        }

        a = *reinterpret_cast<unsigned long const *>( q->h_addr_list[0] );
    }

    sk_ = ::socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

    if( sk_ == INVALID_SOCKET )
    {
        throw_socket_error( name_, "socket create error", WSAGetLastError() );
    }

    sockaddr_in addr = { AF_INET };

    addr.sin_addr.s_addr = a;
    addr.sin_port = htons( static_cast<u_short>( port ) );

    int r = ::connect( sk_, (sockaddr const *)&addr, sizeof(addr) );

    if( r != 0 )
    {
        int r2 = WSAGetLastError();

        closesocket( sk_ );

        throw_socket_error( name_, "TCP connect error", r2 );
    }
}

tcp_reader::~tcp_reader()
{
    shutdown( sk_, 2 );
    closesocket( sk_ );
}

std::string tcp_reader::name() const
{
    return name_;
}

std::size_t tcp_reader::read( void * p, std::size_t n )
{
    assert( n <= INT_MAX );

    int r = ::recv( sk_, static_cast< char* >( p ), n, 0 );

    if( r < 0 )
    {
        throw_socket_error( name(), "TCP receive error", WSAGetLastError() );
    }

    return r;
}

void tcp_reader::write( void const * p, std::size_t n )
{
    assert( n <= INT_MAX );

    int r = ::send( sk_, static_cast< char const* >( p ), n, 0 );

    if( r < 0 || r < n )
    {
        throw_socket_error( name(), "TCP send error", WSAGetLastError() );
    }
}

#if defined( _WIN32 )

static WSADATA s_wd;
static int s_wsa_startup = WSAStartup( MAKEWORD( 1, 1 ), &s_wd );

#endif // defined( _WIN32 )

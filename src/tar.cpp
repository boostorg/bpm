//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "tar.hpp"
#include "error.hpp"
#include "fs.hpp"
#include "message.hpp"
#include <cstdio>
#include <cstddef>
#include <cassert>
#include <ctime>
#include <cstring>
#include <errno.h>

static void throw_eof_error( std::string const & name )
{
    throw_error( name, "unexpected end of file" );
}

static bool contains_dotdot( std::string const & fn )
{
    std::size_t i = 0;

    for( ;; )
    {
        std::size_t j = fn.find( '/', i );

        if( j == std::string::npos )
        {
            return fn.substr( i ) == "..";
        }

        if( fn.substr( i, j - i ) == ".." )
        {
            return true;
        }

        i = j + 1;
    }
}

int const N = 512;

static bool read_header( basic_reader * pr, char (&header)[ N ], std::string & fn, char & type, long long & size, int & mode, long long & mtime )
{
    {
        std::size_t r = pr->read( header, N );

        if( r == 0 ) return true; // EOF

        if( r < N )
        {
            throw_eof_error( pr->name() );
        }
    }

    if( header[0] == 0 )
    {
        // empty block

        unsigned m = 0;

        for( int i = 0; i < N; ++i )
        {
            m |= header[ i ];
        }

        if( m != 0 )
        {
            throw_error( pr->name(), "bad block" );
        }

        return false;
    }

    // check header checksum

    int checksum = 0;
    std::sscanf( header + 148, "%8o", &checksum );

    {
        int s = 0;

        std::memset( header + 148, ' ', 8 );

        for( int i = 0; i < N; ++i )
        {
            s += static_cast< unsigned char >( header[ i ] );
        }

        if( s != checksum )
        {
            throw_error( pr->name(), "header checksum mismatch" );
        }
    }

    header[ 99 ] = 0;
    fn = header;

    header[ 257 + 5 ] = 0;
    std::string magic( header + 257 );

    if( magic == "ustar" )
    {
        header[ 345 + 130 ] = 0;
        std::string prefix( header + 345 );

        fn = prefix + fn;
    }

    type = header[ 156 ];

    size = 0;
    std::sscanf( header + 124, "%12llo", &size );

    mode = 0;
    std::sscanf( header + 100, "%8o", &mode );

    mtime = 0;
    std::sscanf( header + 136, "%12llo", &mtime );

    return false;
}

static void read_long_name( basic_reader * pr, long long size, std::string & fn )
{
    fn.clear();

    long long k = 0;

    while( k < size )
    {
        char data[ N ];

        {
            std::size_t r = pr->read( data, N );

            if( r < N )
            {
                throw_eof_error( pr->name() );
            }
        }

        unsigned m;

        if( k + N <= size )
        {
            m = N;
        }
        else
        {
            assert( size - k <= UINT_MAX );
            assert( size - k <= N );

            m = static_cast< unsigned >( size - k );
        }

        fn.append( data, m );

        k += N;
    }
}

void tar_extract( basic_reader * pr, std::string const & prefix, std::set< std::string > const & whitelist )
{
    msg_printf( 1, "extracting from '%s'", pr->name().c_str() );

    for( ;; )
    {
        char header[ N ];

        std::string fn;

        char type = 0;
        long long size = 0;
        int mode = 0;
        long long mtime = 0;

        if( read_header( pr, header, fn, type, size, mode, mtime ) )
        {
            break; // EOF
        }

        if( header[ 0 ] == 0 )
        {
            continue;
        }

        if( type == 'L' )
        {
            // GNU long name extension

            read_long_name( pr, size, fn );

            std::string fn2;

            if( read_header( pr, header, fn2, type, size, mode, mtime ) )
            {
                throw_eof_error( pr->name() );
            }
        }

        if( ( fn.substr( 0, prefix.size() ) != prefix && whitelist.count( fn ) == 0 ) || contains_dotdot( fn ) )
        {
            throw_error( pr->name(), "disallowed file name: '" + fn + "'" );
        }

        if( type != 0 && type != '0' && type != '5' )
        {
            throw_error( pr->name(), "disallowed file type" );
        }

        msg_printf( 2, "extracting '%s'", fn.c_str() );

        if( type == '5' ) // directory
        {
            if( size != 0 )
            {
                throw_error( pr->name(), "directory with nonzero size: '" + fn + "'" );
            }

            int r = fs_mkdir( fn, 0755 );

            if( r < 0 )
            {
                throw_errno_error( fn, "create error", errno );
            }

            if( fn != prefix )
            {
                fs_utime( fn, mtime, std::time( 0 ) );
            }
        }
        else // regular file
        {
            int fd = fs_creat( fn, 0644 );

            if( fd < 0 )
            {
                throw_errno_error( fn, "create error", errno );
            }

            long long k = 0;

            while( k < size )
            {
                char data[ N ];

                {
                    std::size_t r = pr->read( data, N );

                    if( r < N )
                    {
                        throw_eof_error( pr->name() );
                    }
                }

                unsigned m;

                if( k + N <= size )
                {
                    m = N;
                }
                else
                {
                    assert( size - k <= UINT_MAX );
                    assert( size - k <= N );

                    m = static_cast< unsigned >( size - k );
                }

                int r = fs_write( fd, data, m );

                if( r < 0 )
                {
                    int r2 = errno;

                    fs_close( fd );
                    throw_errno_error( fn, "write error", r2 );
                }

                if( static_cast< unsigned >( r ) != m )
                {
                    fs_close( fd );
                    throw_errno_error( fn, "write error", ENOSPC );
                }

                k += N;
            }

            fs_close( fd );
            fs_utime( fn, mtime, std::time( 0 ) );
        }
    }
}

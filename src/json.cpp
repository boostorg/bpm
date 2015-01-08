//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "json.hpp"
#include "error.hpp"
#include <fstream>
#include <errno.h>

static void throw_parse_error( std::string const & name, std::string const & reason )
{
    throw_error( name, "parse error: " + reason );
}

static void throw_eof_error( std::string const & name )
{
    throw_parse_error( name, "unexpected end of data" );
}

static void throw_expected_error( std::string const & name, std::string const & expected, std::string const & tk )
{
    throw_parse_error( name, "expected " + expected + ", got '" + tk + "'" );
}

static bool is_structural( char ch )
{
    return ch == '{' || ch == '}' || ch == '[' || ch == ']' || ch == ':' || ch == ',' || ch == 0;
}

static bool is_literal( char ch )
{
    return ( ch >= 'a' && ch <= 'z' ) || ( ch >= 'A' && ch <= 'Z' ) || ch == '_' || ( ch >= '0' && ch <= '9' ) || ch == '-' || ch == '+' || ch == '.';
}

static bool is_whitespace( char ch )
{
    return ch == 0x09 || ch == 0x0A || ch == 0x0D || ch == ' ';
}

static void utf8_encode( std::string & r, unsigned c )
{
    if( c < 0x80 )
    {
        r.push_back( static_cast<unsigned char>( c ) );
    }
    else if( c < 0x800 )
    {
        r.push_back( static_cast<unsigned char>( 0xC0 | (c >>   6) ) );
        r.push_back( static_cast<unsigned char>( 0x80 | (c & 0x3F) ) );
    }
    else // if( c < 0x10000 )
    {
        r.push_back( static_cast<unsigned char>( 0xE0 | (c >> 12)         ) );
        r.push_back( static_cast<unsigned char>( 0x80 | ((c >> 6) & 0x3F) ) );
        r.push_back( static_cast<unsigned char>( 0x80 | (c & 0x3F)        ) );
    }
}

static std::string get_token( std::istream & is, std::string const & name )
{
    char ch;

    std::string tk;

    if( !( is >> ch ) )
    {
        // EOF
    }
    else if( is_literal( ch ) )
    {
        for( ;; )
        {
            tk += ch;

            if( !is.get( ch ) )
            {
                throw_eof_error( name );
            }

            if( is_structural( ch ) || is_whitespace( ch ) )
            {
                is.putback( ch );
                break;
            }
        }
    }
    else if( ch != '"' )
    {
        tk += ch;
    }
    else
    {
        // string

        tk += ch;

        for( ;; )
        {
            if( !is.get( ch ) )
            {
                throw_eof_error( name );
            }

            if( ch == '"' )
            {
                break;
            }

            if( ch != '\\' )
            {
                tk += ch;
                continue;
            }

            if( !is.get( ch ) )
            {
                throw_eof_error( name );
            }

            switch( ch )
            {
            case 'b': tk += '\x08'; break;
            case 'f': tk += '\x0C'; break;
            case 'r': tk += '\x0D'; break;
            case 'n': tk += '\x0A'; break;
            case 't': tk += '\x09'; break;
            default:  tk += ch; break;

            case 'u':
                {
                    char buffer[ 5 ] = { 0 };
                    int code = 0;

                    if( !is.read( buffer, 4 ) || sscanf( buffer, "%x", &code ) != 1 || code <= 0 )
                    {
                        throw_parse_error( name, std::string( "invalid character code '\\u" ) + buffer + "'" );
                    }
                    else
                    {
                        utf8_encode( tk, code );
                    }
                }
                break;
            }
        }
    }

    return tk;
}

static void json_parse( std::istream & /*is*/, std::string const & name, std::string tk, std::string & v );
template< class T > static void json_parse( std::istream & is, std::string const & name, std::string tk, std::vector< T > & v );
template< class K, class V > static void json_parse( std::istream & is, std::string const & name, std::string tk, std::map< K, V > & m );

// tk is already the result of get_token()
static void json_parse( std::istream & /*is*/, std::string const & name, std::string tk, std::string & v )
{
    char ch = tk[ 0 ];

    if( is_structural( ch ) )
    {
        throw_expected_error( name, "string", tk );
    }

    if( tk[ 0 ] == '"' )
    {
        v = tk.substr( 1 );
    }
    else
    {
        v = tk; // true, false, null, or number
    }
}

// tk is already the result of get_token()
template< class T > static void json_parse( std::istream & is, std::string const & name, std::string tk, std::vector< T > & v )
{
    if( tk != "[" )
    {
        // assume single element

        T t;
        json_parse( is, name, tk, t );

        v.push_back( t );

        return;
    }

    for( ;; )
    {
        tk = get_token( is, name );

        if( tk == "]" )
        {
            break;
        }

        {
            T t;
            json_parse( is, name, tk, t );

            v.push_back( t );
        }

        tk = get_token( is, name );

        if( tk == "]" )
        {
            break;
        }

        if( tk != "," )
        {
            throw_expected_error( name, "',' or ']'", tk );
        }
    }
}

// tk is already the result of get_token()
template< class K, class V > static void json_parse( std::istream & is, std::string const & name, std::string tk, std::map< K, V > & m )
{
    if( tk != "{" )
    {
        throw_expected_error( name, "'{'", tk );
    }

    for( ;; )
    {
        tk = get_token( is, name );

        if( tk == "}" )
        {
            break;
        }

        K k;
        json_parse( is, name, tk, k );

        tk = get_token( is, name );

        if( tk != ":" )
        {
            throw_expected_error( name, "':'", tk );
            return;
        }

        tk = get_token( is, name );

        V v;
        json_parse( is, name, tk, v );

        m[ k ] = v;

        tk = get_token( is, name );

        if( tk == "}" )
        {
            break;
        }

        if( tk != "," )
        {
            throw_expected_error( name, "',' or ']'", tk );
        }
    }
}

void read_libraries_json( std::string const & name, std::vector< std::map< std::string, std::vector< std::string > > > & libraries )
{
    std::ifstream is( name.c_str() );

    if( !is )
    {
        throw_errno_error( name, "open error", errno );
    }

    std::string tk;
    tk = get_token( is, name );

    json_parse( is, name, tk, libraries );
}

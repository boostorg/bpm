//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "cmd_index.hpp"
#include "options.hpp"
#include "message.hpp"
#include "error.hpp"
#include "json.hpp"
#include "fs.hpp"
#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <errno.h>

static void handle_option( std::string const & opt )
{
    if( opt == "-v" )
    {
        increase_message_level();
    }
    else if( opt == "-q" )
    {
        decrease_message_level();
    }
    else
    {
        throw std::runtime_error( "invalid index option: '" + opt + "'" );
    }
}

void cmd_index( char const * argv[] )
{
    parse_options( argv, handle_option );

    if( *argv )
    {
        throw std::runtime_error( std::string( "unexpected argument '" ) + *argv + "'" );
    }

    cmd_index();
}

typedef std::map< std::string, std::vector< std::string > > library;

static std::string to_string( std::vector< std::string > const & v )
{
    std::string r;

    for( std::size_t i = 0, n = v.size(); i < n; ++i )
    {
        if( i != 0 )
        {
            r += ", ";
        }

        r += v[ i ];
    }

    return r;
}

static std::string get_field( library const & lib, std::string const & name )
{
    library::const_iterator i = lib.find( name );

    if( i == lib.end() )
    {
        return std::string();
    }
    else
    {
        return to_string( i->second );
    }
}

static void set_field( library & lib, std::string const & name, std::string const & value )
{
    lib[ name ].resize( 1 );
    lib[ name ].front() = value;
}

static bool has_meta_file( std::string const & path )
{
    return fs_exists( path + "/meta/libraries.json" ) ||
           fs_exists( path + "/.boost" );
}

static std::string find_meta_file( std::string const & path )
{
    bool meta_libraries = fs_exists(path + "/meta/libraries.json");
    bool dot_boost = fs_exists(path + "/.boost");

    if( meta_libraries && dot_boost )
    {
        msg_printf( -2, "%s contains both a meta/libraries.json file and a .boost file", path.c_str() );
    }

    if( !meta_libraries && !dot_boost )
    {
        msg_printf( -2, "%s contains neither a meta/libraries.json file nor a .boost file", path.c_str() );
    }

    return path + (dot_boost ? "/.boost" : "/meta/libraries.json");
}

static void add_library( std::string const & path, std::map< std::string, library > & libraries, std::map< std::string, std::vector< std::string > > & categories )
{
    std::string name = find_meta_file( path );

    msg_printf( 2, "reading '%s'", name.c_str() );

    std::vector< library > v;

    try
    {
        read_libraries_json( name, v );
    }
    catch( std::exception const & x )
    {
        msg_printf( -2, "%s", x.what() );
    }

    for( std::vector< library >::iterator i = v.begin(); i != v.end(); ++i )
    {
        library & lib = *i;

        std::string key = get_field( lib, "key" );

        if( key.empty() )
        {
            key = path;

            int j = i - v.begin();

            if( j != 0 )
            {
                char buffer[ 32 ];
                sprintf( buffer, ".%d", j + 1 );

                key += buffer;
            }

            msg_printf( -1, "'%s': library has no key, assuming '%s'", name.c_str(), key.c_str() );

            set_field( lib, "key", key );
        }

        std::string fname = get_field( lib, "name" );

        if( fname.empty() )
        {
            msg_printf( -1, "'%s': library '%s' has no name", name.c_str(), key.c_str() );
        }

        set_field( lib, "path", path );

        std::vector< std::string > const & cv = lib[ "category" ];

        libraries[ key ] = lib;

        for( std::vector< std::string >::const_iterator j = cv.begin(); j != cv.end(); ++j )
        {
            categories[ *j ].push_back( key );
        }
    }
}

static void build_library_map( std::string const & path, std::map< std::string, library > & libraries, std::map< std::string, std::vector< std::string > > & categories )
{
    // enumerate modules in 'path'

    std::vector< std::string > entries;
    int r = fs_readdir( path, entries );

    if( r != 0 )
    {
        throw_errno_error( path, "read error", errno );
    }

    for( std::vector< std::string >::const_iterator i = entries.begin(); i != entries.end(); ++i )
    {
        if( *i == "." || *i == ".." ) continue;

        std::string p2 = path + "/" + *i;

        if( fs_is_dir( p2 ) && has_meta_file( p2 ) )
        {
            add_library( p2, libraries, categories );
        }

        if( fs_exists( p2 + "/sublibs" ) )
        {
            // enumerate submodules
            build_library_map( p2, libraries, categories );
        }
    }
}

static void write_index( std::string const & name, std::map< std::string, library > const & libraries, std::map< std::string, std::vector< std::string > > const & categories );

void cmd_index()
{
    msg_printf( 0, "recreating index" );

    std::map< std::string, library > libraries;
    std::map< std::string, std::vector< std::string > > categories;

    build_library_map( "libs", libraries, categories );

    write_index( "index.html", libraries, categories );
}

static char const * file_header =

"<!DOCTYPE HTML>\n"
"<html>\n"
"\n"
"<head>\n"
"\n"
"<title>Installed Boost C++ Libraries</title>\n"
"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n"
"\n"
"<style type=\"text/css\">\n"
"\n"
"A { color: #06C; text-decoration: none; }\n"
"A:hover { text-decoration: underline; }\n"
"\n"
".main { margin-left: 4em; margin-right: 4em; color: #4A6484; }\n"
"\n"
".logo { font-family: sans-serif; font-style: italic; }\n"
".logo .upper { font-size: 48pt; font-weight: 800; }\n"
".logo .lower { font-size: 17pt; }\n"
"\n"
".header { margin-top: 2em; }\n"
".section { margin-top: 2em; }\n"
"\n"
".category-container { margin-left: 1em; padding-left: 1em; border-left: 1px dotted; }\n"
"\n"
"dt { float: left; clear: left; width: 6em; }\n"
"dd { margin-left: 7em; }\n"
"\n"
"#alphabetically p { margin-left: 2em; }\n"
"#alphabetically dl { margin-left: 2em; }\n"
"\n"
"</style>\n"
"\n"
"</head>\n"
"\n"
"<body>\n"
"<div class=\"main\">\n"
"\n"
"<div class=\"logo\">\n"
"<div class=\"upper\">boost</div>\n"
"<div class=\"lower\">C++ LIBRARIES</div>\n"
"</div>\n\n";

static void write_category_link( std::ostream & os, std::string const & name, std::string const & id )
{
    os << "<div><a href=\"#cat:" + id + "\">" + name + "</a></div>\n";
}

static void write_header( std::ostream & os, std::map< std::string, std::pair< std::string, std::vector< std::string > > > const & categories )
{
    os << "<div class=\"header\">\n"
          "<div><a href=\"#alphabetically\">Libraries Listed Alphabetically</a></div>\n"
          "<div><a href=\"#by_category\">Libraries Listed by Category</a></div>\n"
          "<div>&nbsp;</div>\n";

    for( std::map< std::string, std::pair< std::string, std::vector< std::string > > >::const_iterator i = categories.begin(); i != categories.end(); ++i )
    {
        write_category_link( os, i->first, i->second.first );
    }

    os << "</div>\n\n";
}

static std::string category_name( std::string const & cat );

static void write_alpha_library( std::ostream & os, library const & lib )
{
    os << "<h4><a href=\"http://www.boost.org/" << get_field( lib, "path" ) << "/" << get_field( lib, "documentation" ) << "\">" << get_field( lib, "name" ) << "</a></h4>\n\n"

          "<p class=\"description\">" << get_field( lib, "description" ) << "</p>\n\n"

          "<dl>\n";

    std::string authors = get_field( lib, "authors" );

    if( !authors.empty() )
    {
        os << "<dt>Author(s):</dt><dd>" << authors << "</dd>\n";
    }

    {
        library::const_iterator i = lib.find( "category" );

        if( i != lib.end() )
        {
            os << "<dt>Category:</dt><dd>";

            int k = 0;

            for( std::vector< std::string>::const_iterator j = i->second.begin(); j != i->second.end(); ++j, ++k )
            {
                if( k != 0 )
                {
                    os << " &bull; ";
                }

                os << "<a href=\"#cat:" << *j << "\">" << category_name( *j ) << "</a>";
            }

            os << "</dd>\n";
        }
    }

    os << "</dl>\n\n";
}

static void write_alpha_section( std::ostream & os, std::map< std::string, library > const & libraries )
{
    os << "<div class=\"section\" id=\"alphabetically\">\n"
          "\n"
          "<h2>Libraries Listed Alphabetically</h2>\n\n";

    // re-sort by name
    std::map< std::string, library > lib2;

    for( std::map< std::string, library >::const_iterator i = libraries.begin(); i != libraries.end(); ++i )
    {
        std::string name = get_field( i->second, "name" );

        if( name.empty() ) continue;

        lib2[ name ] = i->second;
    }

    for( std::map< std::string, library >::const_iterator i = lib2.begin(); i != lib2.end(); ++i )
    {
        write_alpha_library( os, i->second );
    }

    os << "</div>\n\n";
}

static void write_cat_library( std::ostream & os, library const & lib )
{
    os << "<h4><a href=\"http://www.boost.org/" << get_field( lib, "path" ) << "/" << get_field( lib, "documentation" ) << "\">" << get_field( lib, "name" ) << "</a></h4>\n\n"

          "<p class=\"description\">" << get_field( lib, "description" ) << "</p>\n\n";
}

static void write_category( std::ostream & os, std::map< std::string, library > const & libraries, std::string const & name, std::string const & id, std::vector< std::string > const & libs )
{
    os << "<h3 class=\"category\" id=\"cat:" << id << "\">" << name << "</h3>\n\n"
          "<div class=\"category-container\">\n\n";

    for( std::vector< std::string >::const_iterator i = libs.begin(); i != libs.end(); ++i )
    {
        std::map< std::string, library >::const_iterator j = libraries.find( *i );

        if( j != libraries.end() )
        {
            write_cat_library( os, j->second );
        }
    }

    os << "</div>\n\n";
}

static void write_cat_section( std::ostream & os, std::map< std::string, library > const & libraries, std::map< std::string, std::pair< std::string, std::vector< std::string > > > const & categories )
{
    os << "<div class=\"section\" id=\"by_category\">\n"
          "\n"
          "<h2>Libraries Listed by Category</h2>\n\n";

    for( std::map< std::string, std::pair< std::string, std::vector< std::string > > >::const_iterator i = categories.begin(); i != categories.end(); ++i )
    {
        write_category( os, libraries, i->first, i->second.first, i->second.second );
    }

    os << "</div>\n\n";
}

static char const * file_footer =

"</div>\n"
"</body>\n"
"</html>\n";

static std::string category_name( std::string const & cat )
{
    static char const * table[][ 2 ] =
    {
        { "String", "String and Text Processing" },
        { "Function-objects", "Function Objects and Higher-Order Programming" },
        { "Generic", "Generic Programming" },
        { "Metaprogramming", "Template Metaprogramming" },
        { "Preprocessor", "Preprocessor Metaprogramming" },
        { "Concurrent", "Concurrent Programming" },
        { "Math", "Math and Numerics" },
        { "Correctness", "Correctness and Testing" },
        { "Data", "Data Structures" },
        { "Domain", "Domain Specific" },
        { "Image-processing", "Image Processing" },
        { "IO", "Input/Output" },
        { "Inter-language", "Inter-Language Support" },
        { "Emulation", "Emulation of Language Features" },
        { "Patterns", "Patterns and Idioms" },
        { "Programming", "Programming Interfaces" },
        { "State", "State Machines" },
        { "workarounds", "Broken Compiler Workarounds" },
    };

    for( int i = 0, n = sizeof( table ) / sizeof( table[0] ); i < n; ++i )
    {
        if( cat == table[ i ][ 0 ] ) return table[ i ][ 1 ];
    }

    return cat;
}

static void write_index( std::string const & name, std::map< std::string, library > const & libraries, std::map< std::string, std::vector< std::string > > const & categories )
{
    msg_printf( 2, "writing '%s'", name.c_str() );

    std::ofstream os( name.c_str() );

    if( !os )
    {
        throw_errno_error( name, "open error", errno );
    }

    os << file_header;

    // re-sort categories by name

    std::map< std::string, std::pair< std::string, std::vector< std::string > > > cats;

    for( std::map< std::string, std::vector< std::string > >::const_iterator i = categories.begin(); i != categories.end(); ++i )
    {
        cats[ category_name( i->first ) ] = *i;
    }

    write_header( os, cats );
    write_alpha_section( os, libraries );
    write_cat_section( os, libraries, cats );

    os << file_footer;
}

#ifndef FS_HPP_INCLUDED
#define FS_HPP_INCLUDED

//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include <string>
#include <vector>
#include <ctime>

int fs_creat( std::string const & path, int mode );
int fs_write( int fd, void const * buffer, unsigned n );
int fs_close( int fd );

int fs_mkdir( std::string const & path, int mode );

bool fs_exists( std::string const & path );

int fs_stat( std::string const & path, int & mode, std::time_t & mtime, std::time_t & ctime, std::time_t & atime );

std::time_t fs_mtime( std::string const & path );

int fs_utime( std::string const & path, std::time_t mtime, std::time_t atime );

int fs_rmdir( std::string const & path );

void fs_remove_all( std::string const & path, void (*removing)( std::string const & ), void (*error)( std::string const &, int ) );

int fs_readdir( std::string const & path, std::vector< std::string > & entries );

bool fs_is_dir( std::string const & path );

// these two functions treat 'target' as a path name, not as a string, as per POSIX symlink

int fs_link_file( std::string const & link, std::string const & target ); // symlink, if fails on Windows, hard link
int fs_link_dir( std::string const & link, std::string const & target ); // symlink, if fails on Windows, junction

#endif // #ifndef FS_HPP_INCLUDED

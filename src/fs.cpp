//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "fs.hpp"
#include <cstdio>
#include <cassert>
#include <errno.h>

std::time_t fs_mtime( std::string const & path )
{
    int mode;
    std::time_t mtime, ctime, atime;

    if( fs_stat( path, mode, mtime, ctime, atime ) == 0 )
    {
        return mtime;
    }
    else
    {
        return -1;
    }
}

#if defined( _WIN32 )

#define PATH_SEPARATOR '\\'

static bool is_absolute( std::string const & path )
{
    return path[ 0 ] == '\\' || path[ 0 ] == '/' || path.size() >= 2 && path[ 1 ] == ':';
}

static std::size_t find_next_separator( std::string const & path, std::size_t offset )
{
    return path.find_first_of( "/\\", offset );
}

#else

#define PATH_SEPARATOR '/'

static bool is_absolute( std::string const & path )
{
    return path[ 0 ] == '/';
}

static std::size_t find_next_separator( std::string const & path, std::size_t offset )
{
    return path.find( '/', offset );
}

#endif // _WIN32

static int path_element_depth( std::string const & element )
{
    if( element == "" || element == "." )
    {
        return 0;
    }
    else if( element == ".." )
    {
        return -1;
    }
    else
    {
        return +1;
    }
}

static int path_depth( std::string const & path )
{
    int l = 0;

    std::size_t i = 0;

    for( ;; )
    {
        std::size_t j = find_next_separator( path, i );

        if( j == std::string::npos )
        {
            l += path_element_depth( path.substr( i ) );
            break;
        }

        l += path_element_depth( path.substr( i, j - i ) );

        i = j + 1;
    }

    return l;
}

static bool make_relative( std::string const & link, std::string & target )
{
    if( target.empty() )
    {
        return false;
    }

    if( is_absolute( target ) )
    {
        // absolute pathname
        return true;
    }

    if( link.empty() )
    {
        return false;
    }

    if( is_absolute( link ) )
    {
        // absolute link, relative target, not supported
        return true;
    }

    int l = path_depth( link );

    if( l <= 0 )
    {
        return false;
    }

    while( --l > 0 )
    {
        target = PATH_SEPARATOR + target;
        target = ".."  + target;
    }

    return true;
}

#if defined( _WIN32 )

#if defined(_WIN32_WINNT)
#undef _WIN32_WINNT
#endif

#define _WIN32_WINNT 0x501

#include <algorithm>
#include <io.h>
#include <direct.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/utime.h>
#include <windows.h>

int fs_creat( std::string const & path, int mode )
{
    return _open( path.c_str(), _O_CREAT | _O_TRUNC | _O_WRONLY | _O_BINARY,  mode & 0600 );
}

int fs_write( int fd, void const * buffer, unsigned n )
{
    return _write( fd, buffer, n );
}

int fs_close( int fd )
{
    return _close( fd );
}

int fs_mkdir( std::string const & path, int /*mode*/ )
{
    return _mkdir( path.c_str() );
}

bool fs_exists( std::string const & path )
{
    return _access( path.c_str(), 0 ) == 0;
}

int fs_stat( std::string const & path, int & mode, std::time_t & mtime, std::time_t & ctime, std::time_t & atime )
{
    struct _stat st;

    int r = _stat( path.c_str(), &st );

    if( r == 0 )
    {
        mode = st.st_mode;

        mtime = st.st_mtime;
        ctime = st.st_ctime;
        atime = st.st_atime;
    }

    return r;
}

int fs_utime( std::string const & path, std::time_t mtime, std::time_t atime )
{
    _utimbuf ut;

    ut.actime = atime;
    ut.modtime = mtime;

    return _utime( path.c_str(), &ut );
}

int fs_rmdir( std::string const & path )
{
    return _rmdir( path.c_str() );
}

void fs_remove_all( std::string const & path, void (*removing)( std::string const & ), void (*error)( std::string const &, int ) )
{
    removing( path );

    DWORD dw = GetFileAttributesA( path.c_str() );

    if( dw == INVALID_FILE_ATTRIBUTES )
    {
        dw = 0;
    }

    if( dw & FILE_ATTRIBUTE_DIRECTORY )
    {
        if( dw & FILE_ATTRIBUTE_REPARSE_POINT )
        {
            // junction or directory symlink

            int r = _rmdir( path.c_str() );

            if( r != 0 )
            {
                error( path, errno );
            }

            return;
        }
    }
    else
    {
        // file or file symlink

        int r = std::remove( path.c_str() );

        if( r != 0 )
        {
            error( path, errno );
        }

        return;
    }

    // ordinary directory, descend

    assert( dw & FILE_ATTRIBUTE_DIRECTORY );
    assert( ( dw & FILE_ATTRIBUTE_REPARSE_POINT ) == 0 );

    _finddata_t fd;

    intptr_t r = _findfirst( ( path + "/*" ).c_str(), &fd );

    if( r >= 0 )
    {
        do
        {
            std::string name = fd.name;

            if( name != "." && name != ".." )
            {
                fs_remove_all( path + '/' + name, removing, error );
            }
        }
        while( _findnext( r, &fd ) == 0 );

        _findclose( r );

        {
            int r2 = _rmdir( path.c_str() );

            if( r2 != 0 )
            {
                error( path, errno );
            }
        }
    }
    else
    {
        error( path + "/*", errno );
    }
}

int fs_readdir( std::string const & path, std::vector< std::string > & entries )
{
    entries.resize( 0 );

    _finddata_t fd;

    intptr_t r = _findfirst( ( path + "/*" ).c_str(), &fd );

    if( r >= 0 )
    {
        do
        {
            entries.push_back( fd.name );
        }
        while( _findnext( r, &fd ) == 0 );

        _findclose( r );

        return 0;
    }
    else
    {
        return -1;
    }
}

bool fs_is_dir( std::string const & path )
{
    DWORD dw = GetFileAttributesA( path.c_str() );

    if( dw == INVALID_FILE_ATTRIBUTES )
    {
        dw = 0;
    }

    return dw & FILE_ATTRIBUTE_DIRECTORY? true: false;
}

static int errno_from_last_error( DWORD r )
{
    switch( r )
    {
    case ERROR_ACCESS_DENIED:           return EACCES;
    case ERROR_ALREADY_EXISTS:          return EEXIST;
    case ERROR_BAD_NET_NAME:            return ENOENT;
    case ERROR_BAD_NETPATH:             return ENOENT;
    case ERROR_BAD_PATHNAME:            return ENOENT;
    case ERROR_BAD_UNIT:                return ENODEV;
    case ERROR_BROKEN_PIPE:             return EPIPE;
    case ERROR_BUFFER_OVERFLOW:         return ENAMETOOLONG;
    case ERROR_BUSY:                    return EBUSY;
    case ERROR_BUSY_DRIVE:              return EBUSY;
    case ERROR_CANNOT_MAKE:             return EACCES;
    case ERROR_CANTOPEN:                return EIO;
    case ERROR_CANTREAD:                return EIO;
    case ERROR_CANTWRITE:               return EIO;
    case ERROR_CHILD_NOT_COMPLETE:      return ECHILD;
    case ERROR_CURRENT_DIRECTORY:       return EBUSY;
    case ERROR_DEV_NOT_EXIST:           return ENODEV;
    case ERROR_DEVICE_IN_USE:           return EBUSY;
    case ERROR_DIR_NOT_EMPTY:           return ENOTEMPTY;
    case ERROR_DIRECT_ACCESS_HANDLE:    return EPERM;
    case ERROR_DIRECTORY:               return EINVAL;
    case ERROR_DISK_FULL:               return ENOSPC;
    case ERROR_DRIVE_LOCKED:            return EACCES;
    case ERROR_FAIL_I24:                return EIO;
    case ERROR_FILE_EXISTS:             return EEXIST;
    case ERROR_FILE_NOT_FOUND:          return ENOENT;
    case ERROR_FILENAME_EXCED_RANGE:    return ENAMETOOLONG;
    case ERROR_HANDLE_DISK_FULL:        return ENOSPC;
    case ERROR_INVALID_ACCESS:          return EACCES;
    case ERROR_INVALID_DRIVE:           return ENODEV;
    case ERROR_INVALID_FUNCTION:        return ENOSYS;
    case ERROR_INVALID_HANDLE:          return EBADF;
    case ERROR_INVALID_NAME:            return EINVAL;
    case ERROR_INVALID_TARGET_HANDLE:   return EBADF;
    case ERROR_LOCK_FAILED:             return EACCES;
    case ERROR_LOCK_VIOLATION:          return EACCES;
    case ERROR_LOCKED:                  return EACCES;
    case ERROR_MAX_THRDS_REACHED:       return EAGAIN;
    case ERROR_NEGATIVE_SEEK:           return EINVAL;
    case ERROR_NETWORK_ACCESS_DENIED:   return EACCES;
    case ERROR_NOACCESS:                return EFAULT;
    case ERROR_NO_MORE_FILES:           return ENOENT;
    case ERROR_NO_PROC_SLOTS:           return EAGAIN;
    case ERROR_NOT_ENOUGH_MEMORY:       return ENOMEM;
    case ERROR_NOT_ENOUGH_QUOTA:        return ENOMEM;
    case ERROR_NOT_LOCKED:              return EACCES;
    case ERROR_NOT_READY:               return EAGAIN;
    case ERROR_NOT_SAME_DEVICE:         return EXDEV;
    case ERROR_OPEN_FAILED:             return EIO;
    case ERROR_OPEN_FILES:              return EBUSY;
    case ERROR_OPERATION_ABORTED:       return EINTR; // ECANCELED;
    case ERROR_OUTOFMEMORY:             return ENOMEM;
    case ERROR_PATH_NOT_FOUND:          return ENOENT;
    case ERROR_READ_FAULT:              return EIO;
    case ERROR_RETRY:                   return EAGAIN;
    case ERROR_SEEK:                    return EIO;
    case ERROR_SEEK_ON_DEVICE:          return ESPIPE;
    case ERROR_SHARING_VIOLATION:       return EACCES;
    case ERROR_TOO_MANY_OPEN_FILES:     return EMFILE;
    case ERROR_WAIT_NO_CHILDREN:        return ECHILD;
    case ERROR_WRITE_FAULT:             return EIO;
    case ERROR_WRITE_PROTECT:           return EROFS;

    default:                            return EINVAL;
    }
}

static void set_errno_from_last_error( DWORD r )
{
    errno = errno_from_last_error( r );
}

typedef BOOLEAN (WINAPI *PtrCreateSymbolicLinkA)(
    /*__in*/ LPCSTR lpSymlinkFileName,
    /*__in*/ LPCSTR lpTargetFileName,
    /*__in*/ DWORD dwFlags
   );

static PtrCreateSymbolicLinkA CreateSymbolicLinkA = PtrCreateSymbolicLinkA( GetProcAddress( GetModuleHandleA( "kernel32.dll" ), "CreateSymbolicLinkA" ) );

#ifndef SYMBOLIC_LINK_FLAG_DIRECTORY
#  define SYMBOLIC_LINK_FLAG_DIRECTORY 1
#endif

int fs_link_file( std::string const & link, std::string const & target )
{
    {
        // CreateSymbolicLink happily accepts '/' separated targets, but the symlink doesn't work

        std::string t2( target );
        std::replace( t2.begin(), t2.end(), '/', '\\' );

        if( !make_relative( link, t2 ) )
        {
            errno = EINVAL;
            return -1;
        }

        if( CreateSymbolicLinkA && CreateSymbolicLinkA( link.c_str(), t2.c_str(), 0 ) )
        {
            return 0;
        }
    }

    if( CreateHardLinkA( link.c_str(), target.c_str(), 0 ) )
    {
        return 0;
    }

    set_errno_from_last_error( GetLastError() );
    return -1;
}

// definitions copied from Boost.Filesystem

#if !defined(REPARSE_DATA_BUFFER_HEADER_SIZE)  // mingw winnt.h does provide the defs

#define SYMLINK_FLAG_RELATIVE 1

typedef struct _REPARSE_DATA_BUFFER {
  ULONG  ReparseTag;
  USHORT  ReparseDataLength;
  USHORT  Reserved;
  union {
    struct {
      USHORT  SubstituteNameOffset;
      USHORT  SubstituteNameLength;
      USHORT  PrintNameOffset;
      USHORT  PrintNameLength;
      ULONG  Flags;
      WCHAR  PathBuffer[1];
     } SymbolicLinkReparseBuffer;
    struct {
      USHORT  SubstituteNameOffset;
      USHORT  SubstituteNameLength;
      USHORT  PrintNameOffset;
      USHORT  PrintNameLength;
      WCHAR  PathBuffer[1];
      } MountPointReparseBuffer;
    struct {
      UCHAR  DataBuffer[1];
    } GenericReparseBuffer;
  };
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

#define REPARSE_DATA_BUFFER_HEADER_SIZE \
  FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)

#endif

#ifndef MAXIMUM_REPARSE_DATA_BUFFER_SIZE
#  define MAXIMUM_REPARSE_DATA_BUFFER_SIZE  ( 16 * 1024 )
#endif

#ifndef FSCTL_GET_REPARSE_POINT
#  define FSCTL_GET_REPARSE_POINT 0x900a8
#endif

#ifndef FSCTL_SET_REPARSE_POINT
#  define FSCTL_SET_REPARSE_POINT 0x900a4
#endif

#ifndef IO_REPARSE_TAG_MOUNT_POINT
#  define IO_REPARSE_TAG_MOUNT_POINT 0xA0000003L
#endif

static int create_junction( std::string const & link, std::string const & target )
{
    std::wstring target2;

    {
        wchar_t buffer[ 1024 ] = { 0 };

        int r = MultiByteToWideChar( CP_ACP, 0, target.c_str(), -1, buffer, sizeof( buffer ) / sizeof( buffer[0] ) );

        if( r == 0 )
        {
            errno = ENAMETOOLONG;
            return -1;
        }

        target2 = buffer;

        std::replace( target2.begin(), target2.end(), L'/', L'\\' );

        if( target2.empty() || *target2.rbegin() != L'\\' )
        {
            target2 += L'\\';
        }

        DWORD dw = GetFullPathNameW( target2.c_str(), sizeof( buffer ) / sizeof( buffer[ 0 ] ), buffer, 0 );

        if( dw >= sizeof( buffer ) / sizeof( buffer[ 0 ] ) )
        {
            errno = ENAMETOOLONG;
            return -1;
        }

        if( dw == 0 )
        {
            set_errno_from_last_error( GetLastError() );
            return -1;
        }

        target2 = buffer;
    }

    {
        int r = _mkdir( link.c_str() );

        if( r != 0 )
        {
            return r;
        }
    }

    HANDLE h = ::CreateFileA( link.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, NULL );

    if( h == INVALID_HANDLE_VALUE )
    {
        DWORD r2 = GetLastError();

        _rmdir( link.c_str() );

        set_errno_from_last_error( r2 );
        return -1;
    }

    std::wstring print_name( target2 );
    size_t print_name_len = print_name.size() * sizeof( wchar_t );

    std::wstring substitute_name = L"\\??\\" + print_name;
    size_t substitute_name_len = substitute_name.size() * sizeof( wchar_t );

    unsigned char buffer[ MAXIMUM_REPARSE_DATA_BUFFER_SIZE ] = { 0 };

    REPARSE_DATA_BUFFER * pb = reinterpret_cast< REPARSE_DATA_BUFFER * >( buffer );

    pb->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
    pb->Reserved = 0;

    wcscpy( pb->MountPointReparseBuffer.PathBuffer, substitute_name.c_str() );
    wcscpy( pb->MountPointReparseBuffer.PathBuffer + substitute_name.size() + 1, print_name.c_str() );

    pb->MountPointReparseBuffer.SubstituteNameOffset = 0;
    pb->MountPointReparseBuffer.SubstituteNameLength = substitute_name_len;

    pb->MountPointReparseBuffer.PrintNameOffset = substitute_name_len + sizeof( wchar_t );
    pb->MountPointReparseBuffer.PrintNameLength = print_name_len;

    pb->ReparseDataLength = FIELD_OFFSET( REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer ) - FIELD_OFFSET( REPARSE_DATA_BUFFER, MountPointReparseBuffer ) + substitute_name_len + sizeof( wchar_t ) + print_name_len + sizeof( wchar_t );

    {
        DWORD dw = 0;
        BOOL r = ::DeviceIoControl( h, FSCTL_SET_REPARSE_POINT, buffer, pb->ReparseDataLength + REPARSE_DATA_BUFFER_HEADER_SIZE, 0, 0, &dw, 0 );

        DWORD r2 = GetLastError();

        CloseHandle( h );

        if( !r )
        {
            _rmdir( link.c_str() );

            set_errno_from_last_error( r2 );
            return -1;
        }
    }

    return 0;
}

int fs_link_dir( std::string const & link, std::string const & target )
{
    {
        // CreateSymbolicLink happily accepts '/' separated targets, but the symlink doesn't work

        std::string t2( target );
        std::replace( t2.begin(), t2.end(), '/', '\\' );

        if( !make_relative( link, t2 ) )
        {
            errno = EINVAL;
            return -1;
        }

        if( CreateSymbolicLinkA && CreateSymbolicLinkA( link.c_str(), t2.c_str(), SYMBOLIC_LINK_FLAG_DIRECTORY ) )
        {
            return 0;
        }
    }

    return create_junction( link, target );
}

#else

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>
#include <dirent.h>

int fs_creat( std::string const & path, int mode )
{
    return creat( path.c_str(), mode );
}

int fs_write( int fd, void const * buffer, unsigned n )
{
    return write( fd, buffer, n );
}

int fs_close( int fd )
{
    return close( fd );
}

int fs_mkdir( std::string const & path, int mode )
{
    return mkdir( path.c_str(), mode );
}

bool fs_exists( std::string const & path )
{
    struct stat st;
    int r = stat( path.c_str(), &st );

    return r == 0;
}

int fs_stat( std::string const & path, int & mode, std::time_t & mtime, std::time_t & ctime, std::time_t & atime )
{
    struct stat st;
    int r = stat( path.c_str(), &st );

    if( r == 0 )
    {
        mode = st.st_mode;

        mtime = st.st_mtime;
        ctime = st.st_ctime;
        atime = st.st_atime;
    }

    return r;
}

int fs_utime( std::string const & path, std::time_t mtime, std::time_t atime )
{
    utimbuf ut;

    ut.actime = atime;
    ut.modtime = mtime;

    return utime( path.c_str(), &ut );
}

int fs_rmdir( std::string const & path )
{
    return rmdir( path.c_str() );
}

void fs_remove_all( std::string const & path, void (*removing)( std::string const & ), void (*error)( std::string const &, int ) )
{
    removing( path );

    if( !fs_is_dir( path ) )
    {
        // file or symlink

        int r = std::remove( path.c_str() );

        if( r != 0 )
        {
            error( path, errno );
        }

        return;
    }

    // ordinary directory, descend

    DIR * pd = opendir( path.c_str() );

    if( pd == 0 )
    {
        error( path + "/*", errno );
        return;
    }

    while( dirent * pe = readdir( pd ) )
    {
        std::string name = pe->d_name;

        if( name != "." && name != ".." )
        {
            fs_remove_all( path + '/' + name, removing, error );
        }
    }

    closedir( pd );

    int r = rmdir( path.c_str() );

    if( r != 0 )
    {
        error( path, errno );
    }
}

int fs_readdir( std::string const & path, std::vector< std::string > & entries )
{
    entries.resize( 0 );

    DIR * pd = opendir( path.c_str() );
    if( pd == 0 ) return -1;

    while( dirent * pe = readdir( pd ) )
    {
        entries.push_back( pe->d_name );
    }

    closedir( pd );
    return 0;
}

bool fs_is_dir( std::string const & path )
{
    struct stat st;
    int r = stat( path.c_str(), &st );

    if( r == 0 )
    {
        return S_ISDIR( st.st_mode );
    }
    else
    {
        return false;
    }
}

int fs_link_file( std::string const & link, std::string const & target )
{
    std::string t2( target );

    if( make_relative( link, t2 ) == 0 && symlink( t2.c_str(), link.c_str() ) == 0 )
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int fs_link_dir( std::string const & link, std::string const & target )
{
    return fs_link_file( link, target );
}

#endif // defined( _WIN32 )

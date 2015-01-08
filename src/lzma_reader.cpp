//
// Copyright 2014, 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

#include "lzma_reader.hpp"
#include "error.hpp"
#include "lzma/LzmaDec.h"
#include <stdlib.h>

void* SzAlloc( void*, size_t size )
{
    return malloc( size );
}

void SzFree( void*, void * address )
{
    free( address );
}

static ISzAlloc s_alloc = { SzAlloc, SzFree };

lzma_reader::lzma_reader( basic_reader * pr ): pr_( pr ), k_( 0 ), m_( 0 )
{
    typedef char assert_large_enough[ sizeof( state_ ) >= sizeof( CLzmaDec )? 1: -1 ];

    {
        std::size_t r = pr_->read( header_, sizeof( header_ ) );

        if( r < sizeof( header_ ) )
        {
            throw_error( pr_->name(), "could not read LZMA header" );
        }
    }

    CLzmaDec * st = (CLzmaDec*)state_;

    LzmaDec_Construct( st );

    {
        SRes r = LzmaDec_Allocate( st, header_, LZMA_PROPS_SIZE, &s_alloc );

        if( r != SZ_OK )
        {
            throw_error( pr_->name(), "could not allocate LZMA state" );
        }
    }

    LzmaDec_Init( st );
}

lzma_reader::~lzma_reader()
{
    CLzmaDec * st = (CLzmaDec*)state_;
    LzmaDec_Free( st, &s_alloc );
}

std::string lzma_reader::name() const
{
    return pr_->name();
}

std::size_t lzma_reader::read( void * p, std::size_t n )
{
    CLzmaDec * st = (CLzmaDec*)state_;
    unsigned char * p2 = static_cast< unsigned char* >( p );

    std::size_t r2 = 0;

    while( n > 0 )
    {
        if( m_ == 0 )
        {
            std::size_t r = pr_->read( buffer_, N );

            if( r == 0 ) return r2;

            k_ = 0;
            m_ = r;
        }

        {
            std::size_t n2 = n;
            std::size_t m2 = m_;

            ELzmaStatus status = LZMA_STATUS_NOT_SPECIFIED;

            SRes r = LzmaDec_DecodeToBuf( st, p2, &n2, buffer_ + k_, &m2, LZMA_FINISH_ANY, &status );

            if( r != 0 )
            {
                return -( 1000 + r );
            }

            k_ += m2;
            m_ -= m2;

            p2 += n2;
            n -= n2;

            r2 += n2;
        }
    }

    return r2;
}

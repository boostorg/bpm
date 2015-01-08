#ifndef MESSAGE_HPP_INCLUDED
#define MESSAGE_HPP_INCLUDED

//
// Copyright 2015 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//

int get_message_level();
void set_message_level( int level );

void increase_message_level();
void decrease_message_level();

void msg_printf( int level, char const * format, ... );

#endif // #ifndef MESSAGE_HPP_INCLUDED

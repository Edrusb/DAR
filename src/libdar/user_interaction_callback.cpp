/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// to contact the author, see the AUTHOR file
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_STRINGS_H
#include <strings.h>
#endif

#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
    char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if STDC_HEADERS
#include <stdarg.h>
#endif
} // end extern "C"

#include <iostream>

#include "user_interaction_callback.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "integers.hpp"
#include "deci.hpp"
#include "nls_swap.hpp"

using namespace std;

namespace libdar
{
    using libdar::Elibcall;
    using libdar::Ebug;
    using libdar::Egeneric;
    using libdar::dar_gettext;
    using libdar::Ememory;

    user_interaction_callback::user_interaction_callback(message_callback x_message_callback,
							 pause_callback x_answer_callback,
							 get_string_callback x_string_callback,
							 get_secu_string_callback x_secu_string_callback,
							 void *context_value)
    {
	NLS_SWAP_IN;
	try
	{
	    if(x_message_callback == nullptr || x_answer_callback == nullptr || x_string_callback == nullptr || x_secu_string_callback == nullptr)
		throw Elibcall("user_interaction_callback::user_interaction_callback", dar_gettext("nullptr given as argument of user_interaction_callback()"));
	    message_cb = x_message_callback;
	    pause_cb  = x_answer_callback;
	    get_string_cb  = x_string_callback;
	    get_secu_string_cb = x_secu_string_callback;
	    context_val = context_value;
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void user_interaction_callback::inherited_message(const string & message)
    {
        if(message_cb == nullptr)
	    throw SRC_BUG;
        else
	{
	    try
	    {
		(*message_cb)(message, context_val);
	    }
	    catch(...)
	    {
		throw Elibcall("user_interaction_callback::inherited_message", "user provided callback function (user_interaction_callback/message) should not throw any exception");
	    }
	}
    }

    bool user_interaction_callback::inherited_pause(const string & message)
    {
        if(pause_cb == nullptr)
	    throw SRC_BUG;
        else
	{
	    try
	    {
		return (*pause_cb)(message, context_val);
	    }
	    catch(...)
	    {
		throw Elibcall("user_interaction_callback::inherited_pause", "user provided callback function (user_interaction_callback/pause) should not throw any exception");
	    }
	}
    }

    string user_interaction_callback::inherited_get_string(const string & message, bool echo)
    {
	if(get_string_cb == nullptr)
	    throw SRC_BUG;
	else
	{
	    try
	    {
		return (*get_string_cb)(message, echo, context_val);
	    }
	    catch(...)
	    {
		throw Elibcall("user_interaction_callback::inherited_get_string", "user provided callback function (user_interaction_callback/get_string) should not throw any exception");
	    }
	}
    }

    secu_string user_interaction_callback::inherited_get_secu_string(const string & message, bool echo)
    {
	if(get_secu_string_cb == nullptr)
	    throw SRC_BUG;
	else
	{
	    try
	    {
		return (*get_secu_string_cb)(message, echo, context_val);
	    }
	    catch(...)
	    {
		throw Elibcall("user_interaction_callback::inherited_get_secu_string", "user provided callback function (user_interaction_callback/get_secu_string) should not throw any exception");
	    }
	}
    }

} // end of namespace

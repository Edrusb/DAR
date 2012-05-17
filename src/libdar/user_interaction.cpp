/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2052 Denis Corbin
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: user_interaction.cpp,v 1.24.2.1 2005/02/20 16:05:47 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
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
#include "user_interaction.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "integers.hpp"
#include "deci.hpp"
#include "nls_swap.hpp"

using namespace std;

namespace libdar
{
    user_interaction_callback::user_interaction_callback(void (*x_warning_callback)(const string &x, void *context),
							 bool (*x_answer_callback)(const string &x, void *context),
							 string (*x_string_callback)(const string &x, bool echo, void *context),
							 void *context_value)
    {
	NLS_SWAP_IN;
	try
	{
	    if(x_warning_callback == NULL || x_answer_callback == NULL)
		throw Elibcall("user_interaction_callback::user_interaction_callback", gettext("NULL given as argument of user_interaction_callback"));
	    warning_callback = x_warning_callback;
	    answer_callback  = x_answer_callback;
	    string_callback  = x_string_callback;
	    tar_listing_callback = NULL;
	    context_val = context_value;
	}
	catch(...)
	{
	    NLS_SWAP_OUT;
	    throw;
	}
	NLS_SWAP_OUT;
    }

    void user_interaction_callback::pause(const string & message)
    {
        if(answer_callback == NULL)
	    throw SRC_BUG;
        else
	{
	    try
	    {
		if(! (*answer_callback)(message, context_val))
		    throw Euser_abort(message);
	    }
	    catch(Euser_abort & e)
	    {
		throw;
	    }
	    catch(...)
	    {
		throw Elibcall("user_interaction_callback::pause", gettext("No exception allowed from libdar callbacks"));
	    }
	}
    }

    void user_interaction_callback::warning(const string & message)
    {
        if(warning_callback == NULL)
	    throw SRC_BUG;
        else
	{
	    try
	    {
		(*warning_callback)(message + '\n', context_val);
	    }
	    catch(...)
	    {
		throw Elibcall("user_interaction_callback::warning", gettext("No exception allowed from libdar callbacks"));
	    }
	}
    }

    string user_interaction_callback::get_string(const string & message, bool echo)
    {
	if(string_callback == NULL)
	    throw SRC_BUG;
	else
	{
	    try
	    {
		return (*string_callback)(message, echo, context_val);
	    }
	    catch(...)
	    {
		throw Elibcall("user_interaction_callback::get_string", gettext("No exception allowed from libdar callbacks"));
	    }
	}
    }

    void user_interaction::listing(const std::string & flag,
				   const std::string & perm,
				   const std::string & uid,
				   const std::string & gid,
				   const std::string & size,
				   const std::string & date,
				   const std::string & filename,
				   bool is_dir,
				   bool has_children)
    {
	    // stupid code to stop having compiler complaining against unused arguments

	throw Elibcall("user_interaction::listing",
		       tools_printf(gettext("Not overwritten listing() method called with: (%S, %S, %S, %S, %S, %S, %S, %s, %s)"),
				    &flag,
				    &perm,
				    &uid,
				    &gid,
				    &size,
				    &date,
				    &filename,
				    is_dir ? "true" : "false",
				    has_children ? "true" : "false"));
    }


	void user_interaction_callback::listing(const string & flag,
					    const string & perm,
					    const string & uid,
					    const string & gid,
					    const string & size,
					    const string & date,
					    const string & filename,
					    bool is_dir,
					    bool has_children)
    {
	if(tar_listing_callback != NULL)
	{
	    try
	    {
		(*tar_listing_callback)(flag, perm, uid, gid, size, date, filename, is_dir, has_children, context_val);
	    }
	    catch(...)
	    {
		throw Elibcall("user_interaction_callback::listing", gettext("No exception allowed from libdar callbacks"));
	    }
	}
    }

    user_interaction * user_interaction_callback::clone() const
    {
	user_interaction *ret = new user_interaction_callback(*this); // copy constructor
	if(ret == NULL)
	    throw Ememory("user_interaction_callback::clone");
	else
	    return ret;
    }


    static void dummy_call(char *x)
    {
        static char id[]="$Id: user_interaction.cpp,v 1.24.2.1 2005/02/20 16:05:47 edrusb Rel $";
        dummy_call(id);
    }

    void user_interaction::printf(const char *format, ...)
    {
	va_list ap;
	va_start(ap, format);
	string output = "";
	try
	{
	    output = tools_vprintf(format, ap);
	}
	catch(...)
	{
	    va_end(ap);
	    throw;
	}
	va_end(ap);
	tools_remove_last_char_if_equal_to('\n', output);
	warning(output);
    }

} // end of namespace


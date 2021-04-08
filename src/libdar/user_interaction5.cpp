/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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

#include "user_interaction5.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "integers.hpp"
#include "deci.hpp"
#include "nls_swap.hpp"

using namespace std;

namespace libdar5
{
    static void remove_last_char_if_equal_to(char c, string & s);

    user_interaction::user_interaction()
    {
	use_listing = false;
	use_dar_manager_show_files = false;
	use_dar_manager_contents = false;
	use_dar_manager_statistics = false;
	use_dar_manager_show_version = false;
	at_once = 0;
	count = 0;
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
		       libdar::tools_printf("Not overwritten listing() method called with: (%S, %S, %S, %S, %S, %S, %S, %s, %s)",
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

    void user_interaction::dar_manager_show_files(const string & filename,
						  bool data_change,
						  bool ea_change)
    {
	throw Elibcall("user_interaction::dar_manager_show_files", "Not overwritten dar_manager_show_files() method has been called!");
    }

    void user_interaction::dar_manager_contents(U_I number,
						const string & chemin,
						const string & archive_name)
    {
	throw Elibcall("user_interaction::dar_manager_contents", "Not overwritten dar_manager_contents() method has been called!");
    }

    void user_interaction::dar_manager_statistics(U_I number,
						  const infinint & data_count,
						  const infinint & total_data,
						  const infinint & ea_count,
						  const infinint & total_ea)
    {
	throw Elibcall("user_interaction::dar_manager_statistics", "Not overwritten dar_manager_statistics() method has been called!");
    }

    void user_interaction::dar_manager_show_version(U_I number,
						    const string & data_date,
						    const string & data_presence,
						    const string & ea_date,
						    const string & ea_presence)
    {
	throw Elibcall("user_interaction::dar_manager_show_version", "Not overwritten dar_manager_show_version() method has been called!");
    }

    void user_interaction::inherited_message(const std::string & message)
    {
	if(at_once > 0)
	{
 	    U_I c = 0, max = message.size();
	    while(c < max)
	    {
		if(message[c] == '\n')
		    count++;
		c++;
	    }
	    count++; // for the implicit \n at end of message
	    if(count >= at_once)
	    {
		count = 0;
		(void)pause(libdar::dar_gettext("Continue? "));
	    }
	}
	inherited_warning(message);
    }

    bool user_interaction::inherited_pause(const string & message)
    {
	bool ret = true;
	try
	{
	    pause(message);
	}
	catch(Euser_abort & e)
	{
	    ret = false;
	}
	return ret;
    }

    string user_interaction::inherited_get_string(const string & message, bool echo)
    {
	return get_string(message, echo);
    }

    secu_string user_interaction::inherited_get_secu_string(const string & message, bool echo)
    {
	return get_secu_string(message, echo);
    }

    void user_interaction::printf(const char *format, ...)
    {
	va_list ap;
	va_start(ap, format);
	string output = "";
	try
	{
	    output = libdar::tools_vprintf(format, ap);
	}
	catch(...)
	{
	    va_end(ap);
	    throw;
	}
	va_end(ap);
	remove_last_char_if_equal_to('\n', output);
	message(output);
    }

    shared_ptr<user_interaction> user_interaction5_clone_to_shared_ptr(user_interaction & dialog)
    {
	user_interaction *tmp = dialog.clone();

	if(tmp == nullptr)
	    throw Ememory("archive::clone_to_shared_ptr");

	    // the pointed to object is newly created here
	    // and never used afterward, thus passing it
	    // as is to a shared_ptr does not lead to an
	    // undefined behavior
	return shared_ptr<user_interaction>(tmp);
    }

    static void remove_last_char_if_equal_to(char c, string & s)
    {
	if(s[s.size()-1] == c)
	    s = string(s.begin(), s.begin()+(s.size() - 1));
    }


} // end of namespace

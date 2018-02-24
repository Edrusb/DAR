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
// to contact the author : http://dar.linux.free.fr/email.html
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

#include "user_interaction_callback5.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "integers.hpp"
#include "deci.hpp"
#include "nls_swap.hpp"

using namespace std;

namespace libdar5
{

    user_interaction_callback::user_interaction_callback(void (*x_warning_callback)(const string &x, void *context),
							 bool (*x_answer_callback)(const string &x, void *context),
							 string (*x_string_callback)(const string &x, bool echo, void *context),
							 secu_string (*x_secu_string_callback)(const string &x, bool echo, void *context),
							 void *context_value)
    {
	NLS_SWAP_IN;
	try
	{
	    if(x_warning_callback == nullptr || x_answer_callback == nullptr)
		throw Elibcall("user_interaction_callback::user_interaction_callback", libdar::dar_gettext("nullptr given as argument of user_interaction_callback()"));
	    warning_callback = x_warning_callback;
	    answer_callback  = x_answer_callback;
	    string_callback  = x_string_callback;
	    secu_string_callback  = x_secu_string_callback;
	    tar_listing_callback = nullptr;
	    dar_manager_show_files_callback = nullptr;
	    dar_manager_contents_callback = nullptr;
	    dar_manager_statistics_callback = nullptr;
	    dar_manager_show_version_callback = nullptr;
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
        if(answer_callback == nullptr)
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
	    catch(Egeneric & e)
	    {
		throw Elibcall("user_interaction_callback::pause", string(libdar::dar_gettext("No exception allowed from libdar callbacks")) + ": " + e.get_message());
	    }
	    catch(...)

	    {
		throw Elibcall("user_interaction_callback::pause", libdar::dar_gettext("No exception allowed from libdar callbacks"));
	    }
	}
    }

    void user_interaction_callback::inherited_warning(const string & message)
    {
        if(warning_callback == nullptr)
	    throw SRC_BUG;
        else
	{
	    try
	    {
		(*warning_callback)(message + '\n', context_val);
	    }
	    catch(Egeneric & e)
	    {
		throw Elibcall("user_interaction_callback::warning", string(libdar::dar_gettext("No exception allowed from libdar callbacks")) + ": " + e.get_message());
	    }
	    catch(...)
	    {
		throw Elibcall("user_interaction_callback::warning", libdar::dar_gettext("No exception allowed from libdar callbacks"));
	    }
	}
    }

    string user_interaction_callback::get_string(const string & message, bool echo)
    {
	if(string_callback == nullptr)
	    throw SRC_BUG;
	else
	{
	    try
	    {
		return (*string_callback)(message, echo, context_val);
	    }
	    catch(Egeneric & e)
	    {
		throw Elibcall("user_interaction_callback::get_string", string(libdar::dar_gettext("No exception allowed from libdar callbacks")) + ": " + e.get_message());
	    }
	    catch(...)
	    {
		throw Elibcall("user_interaction_callback::get_string", libdar::dar_gettext("No exception allowed from libdar callbacks"));
	    }
	}
    }

    secu_string user_interaction_callback::get_secu_string(const string & message, bool echo)
    {
	if(string_callback == nullptr)
	    throw SRC_BUG;
	else
	{
	    try
	    {
		return (*secu_string_callback)(message, echo, context_val);
	    }
	    catch(Ebug & e)
	    {
		throw;
	    }
	    catch(Egeneric & e)
	    {
		throw Elibcall("user_interaction_callback::get_secu_string", string(libdar::dar_gettext("No exception allowed from libdar callbacks")) + ": " + e.get_message());
	    }
	    catch(...)
	    {
		throw Elibcall("user_interaction_callback::get_secu_string", libdar::dar_gettext("No exception allowed from libdar callbacks"));
	    }
	}
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
	if(tar_listing_callback != nullptr)
	{
	    try
	    {
		(*tar_listing_callback)(flag, perm, uid, gid, size, date, filename, is_dir, has_children, context_val);
	    }
	    catch(Egeneric & e)
	    {
		throw Elibcall("user_interaction_callback::listing", string(libdar::dar_gettext("No exception allowed from libdar callbacks")) + ": " + e.get_message());
	    }
	    catch(...)
	    {
		throw Elibcall("user_interaction_callback::listing", libdar::dar_gettext("No exception allowed from libdar callbacks"));
	    }
	}
    }

    void user_interaction_callback::dar_manager_show_files(const string & filename,
							   bool available_data,
							   bool available_ea)
    {
	if(dar_manager_show_files_callback != nullptr)
	{
	    try
	    {
		(*dar_manager_show_files_callback)(filename, available_data, available_ea, context_val);
	    }
	    catch(Egeneric & e)
	    {
		throw Elibcall("user_interaction_callback::dar_manager_show_files", string(libdar::dar_gettext("No exception allowed from libdar callbacks")) + ": " + e.get_message());
	    }
	    catch(...)
	    {
		throw Elibcall("user_interaction_callback::dar_manager_show_files", libdar::dar_gettext("No exception allowed from libdar callbacks"));
	    }
	}
    }

    void user_interaction_callback::dar_manager_contents(U_I number,
							 const std::string & chemin,
							 const std::string & archive_name)
    {
	if(dar_manager_contents_callback != nullptr)
	{
	    try
	    {
		(*dar_manager_contents_callback)(number, chemin, archive_name, context_val);
	    }
	    catch(Egeneric & e)
	    {
		throw Elibcall("user_interaction_callback::dar_manager_contents", string(libdar::dar_gettext("No exception allowed from libdar callbacks")) + ": " + e.get_message());
	    }
	    catch(...)
	    {
		throw Elibcall("user_interaction_callback::dar_manager_contents", libdar::dar_gettext("No exception allowed from libdar callbacks"));
	    }
	}
    }

    void user_interaction_callback::dar_manager_statistics(U_I number,
							   const infinint & data_count,
							   const infinint & total_data,
							   const infinint & ea_count,
							   const infinint & total_ea)
    {
	if(dar_manager_statistics_callback != nullptr)
	{
	    try
	    {
		(*dar_manager_statistics_callback)(number, data_count, total_data, ea_count, total_ea, context_val);
	    }
	    catch(Egeneric & e)
	    {
		throw Elibcall("user_interaction_callback::dar_manager_statistics", string(libdar::dar_gettext("No exception allowed from libdar callbacks")) + ": " + e.get_message());
	    }
	    catch(...)
	    {
		throw Elibcall("user_interaction_callback::dar_manager_statistics", libdar::dar_gettext("No exception allowed from libdar callbacks"));
	    }
	}
    }

    void user_interaction_callback::dar_manager_show_version(U_I number,
							     const string & data_date,
							     const string & data_presence,
							     const string & ea_date,
							     const string & ea_presence)
    {
	if(dar_manager_show_version_callback != nullptr)
	{
	    try
	    {
		(*dar_manager_show_version_callback)(number, data_date, data_presence, ea_date, ea_presence, context_val);
	    }
	    catch(Egeneric & e)
	    {
		throw Elibcall("user_interaction_callback::dar_manager_show_version", string(libdar::dar_gettext("No exception allowed from libdar callbacks")) + ": " + e.get_message());
	    }
	    catch(...)
	    {
		throw Elibcall("user_interaction_callback::dar_manager_show_version", libdar::dar_gettext("No exception allowed from libdar callbacks"));
	    }
	}
    }


    user_interaction * user_interaction_callback::clone() const
    {
	user_interaction *ret = new (nothrow) user_interaction_callback(*this); // copy constructor
	if(ret == nullptr)
	    throw Ememory("user_interaction_callback::clone");
	else
	    return ret;
    }

} // end of namespace

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

#if HAVE_STRING_H
# include <string.h>
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

} // end extern "C"

#include <iostream>
#include <cstdarg>

#include "user_interaction.hpp"
#include "erreurs.hpp"
#include "tools.hpp"
#include "nls_swap.hpp"

using namespace std;

namespace libdar
{
    void user_interaction::pause(const string & message)
    {
	static const char* errmsg = "user_interaction::inherited_pause should not throw an exception toward libdar";

	bool ret = false;
	try
	{
	    ret = inherited_pause(message);
	}
	catch(Ebug & e)
	{
	    throw;
	}
	catch(Egeneric & e)
	{
	    throw Elibcall("user_interaction::pause", tools_printf("%s : %s", errmsg, e.get_message().c_str()));
	}
	catch(...)
	{
	    throw Elibcall("user_interaction::pause", errmsg);
	}
	if(!ret)
	    throw Euser_abort(message);
    }

    void user_interaction::message(const string & message)
    {
	static const char* errmsg = "user_interaction::inherited_warning should not throw an exception toward libdar";

	try
	{
	    return inherited_message(message);
	}
	catch(Ebug & e)
	{
	    throw;
	}
	catch(Egeneric & e)
	{
	    throw Elibcall("user_interaction::warning", tools_printf("%s : %s", errmsg, e.get_message().c_str()));
	}
	catch(...)
	{
	    throw Elibcall("user_interaction::warning", errmsg);
	}
    }

    string user_interaction::get_string(const string & message, bool echo)
    {
	static const char* errmsg = "user_interaction::inherited_get_string should not throw an exception toward libdar";

	try
	{
	    return inherited_get_string(message, echo);
	}
	catch(Ebug & e)
	{
	    throw;
	}
	catch(Egeneric & e)
	{
	    throw Elibcall("user_interaction::get_string", tools_printf("%s : %s", errmsg, e.get_message().c_str()));
	}
		catch(...)
	{
	    throw Elibcall("user_interaction::get_string", errmsg);
	}
    }

    secu_string user_interaction::get_secu_string(const string & message, bool echo)
    {
	static const char* errmsg = "user_interaction::inherited_get_secu_string should not throw an exception toward libdar";

	try
	{
	    return inherited_get_secu_string(message, echo);
	}
	catch(Ebug & e)
	{
	    throw;
	}
	catch(Egeneric & e)
	{
	    throw Elibcall("user_interaction::get_secu_string", tools_printf("%s : %s", errmsg, e.get_message().c_str()));
	}
	catch(...)
	{
	    throw Elibcall("user_interaction::get_secu_string", errmsg);
	}
    }

    void user_interaction::printf(const char *format, ...)
    {
        va_list ap;
        va_start(ap, format);
        try
        {
            message(tools_vprintf(format, ap));
        }
        catch(...)
        {
            va_end(ap);
            throw;
        }
        va_end(ap);
    }

} // end of namespace

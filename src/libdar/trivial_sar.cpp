//*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2018 Denis Corbin
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
// this was necessary to compile under Mac OS-X (boggus dirent.h)
#if HAVE_STDINT_H
#include <stdint.h>
#endif

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

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

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if STDC_HEADERS
#include <stdlib.h>
#endif


} // end extern "C"

#include "sar.hpp"
#include "deci.hpp"
#include "user_interaction.hpp"
#include "tools.hpp"
#include "erreurs.hpp"
#include "cygwin_adapt.hpp"
#include "deci.hpp"
#include "entrepot.hpp"
#include "sar_tools.hpp"
#include "trivial_sar.hpp"
#include "tuyau.hpp"
#include "fichier_global.hpp"

using namespace std;

namespace libdar
{

    trivial_sar::trivial_sar(const shared_ptr<user_interaction> & dialog,
			     gf_mode open_mode,
			     const std::string & base_name,
			     const std::string & extension,
			     const entrepot & where,
			     const label & internal_name,
			     const label & data_name,
			     const std::string & execute,
			     bool allow_over,
			     bool warn_over,
			     bool force_permission,
			     U_I permission,
			     hash_algo x_hash,
			     const infinint & x_min_digits,
			     bool format_07_compatible) : generic_file(open_mode), mem_ui(dialog)
    {

	    // some local variables to be used

	fichier_global *tmp = nullptr;
	const string filename = sar_tools_make_filename(base_name, 1, x_min_digits, extension);

	    // sanity checks
	if(open_mode == gf_read_only)
	    throw SRC_BUG;

	    // initializing object fields from constructor arguments

	reference = nullptr;
	offset = 0;
	cur_pos = 0;
	end_of_slice = 0;
	hook = execute;
	base = base_name;
	ext = extension;
	of_data_name = data_name;
	old_sar = format_07_compatible;
	min_digits = x_min_digits;
	hook_where = where.get_full_path().display();
	base_url = where.get_url();
	natural_destruction = true;

	    // creating the slice if it does not exist else failing
	try
	{
	    try
	    {
		tmp = where.open(dialog,
				 filename,
				 open_mode,
				 force_permission,
				 permission,
				 true,    //< fail if exists
				 false,   //< erase
				 x_hash);
	    }
	    catch(Esystem & e)
	    {
		switch(e.get_code())
		{
		case Esystem::io_exist:
		    if(tmp != nullptr)
			throw SRC_BUG;

		    if(!allow_over)
			throw Erange("trivial_sar::trivial_sar", tools_printf(gettext("%S already exists, and overwritten is forbidden, aborting"), &filename));
		    if(warn_over)
			get_ui().pause(tools_printf(gettext("%S is about to be overwritten, continue ?"), &filename));

		    try
		    {
			tmp = where.open(dialog,
					 filename,
					 open_mode,
					 force_permission,
					 permission,
					 false,  //< fail if exists
					 true,   //< erase
					 x_hash);

		    }
		    catch(Esystem & e)
		    {
			switch(e.get_code())
			{
			case Esystem::io_exist:
			    throw SRC_BUG;
			case Esystem::io_absent:
			    if(tmp != nullptr)
				throw SRC_BUG;
			    else
			    {
				string tmp = where.get_full_path().display();
				e.prepend_message(tools_printf(gettext("Directory component in %S does not exist or is a dangling symbolic link: "), &tmp));
			    }
			    throw;
			case Esystem::io_access:
			case Esystem::io_ro_fs:
			    e.prepend_message(tools_printf(gettext("Failed creating slice %S: "), &filename));
			    throw; // propagate the exception
			default:
			    throw SRC_BUG;
			}
		    }
		    break;
		case Esystem::io_absent:
		    if(tmp != nullptr)
			throw SRC_BUG;
		    else
		    {
			string tmp = where.get_full_path().display();
			e.prepend_message(tools_printf(gettext("Directory component in %S does not exist or is a dangling symbolic link: "), &tmp));
		    }
		    throw;
		case Esystem::io_access:
		case Esystem::io_ro_fs:
		    e.prepend_message(tools_printf(gettext("Failed creating slice %S: "), &filename));
		    throw; // propagate the exception
		default:
		    if(tmp != nullptr)
			throw SRC_BUG;
		    else
			throw SRC_BUG; // not for the same reason, must know that reporting the same error but on a different line
		}
	    }

	    if(tmp == nullptr)
		throw SRC_BUG;

	    set_info_status(CONTEXT_LAST_SLICE);
	    reference = tmp;
	    init(internal_name);
	    tmp = nullptr; // setting it to null only now was necesary to be able to release the object in case of exception
	}
	catch(...)
	{
	    if(tmp != nullptr)
		delete tmp;
	    throw;
	}

	if(tmp != nullptr)
	    throw SRC_BUG;
    }


    trivial_sar::trivial_sar(const shared_ptr<user_interaction> & dialog,
			     const std::string & pipename,
			     bool lax) : generic_file(gf_read_only) , mem_ui(dialog)
    {
	label for_init;
	reference = nullptr;
	offset = 0;
	cur_pos = 0;
	end_of_slice = 0;
	hook = "";
	base = "";
	ext = "";
	old_sar = false;
	min_digits = 0;
	hook_where = "";
	base_url = "";
	natural_destruction = true;

	set_info_status(CONTEXT_INIT);
	try
	{
	    if(pipename == "-")
		reference = new (nothrow) tuyau(dialog, 0, gf_read_only);
	    else
		reference = new (nothrow) tuyau(dialog, pipename, gf_read_only);

	    if(reference == nullptr)
		throw Ememory("trivial_sar::trivial_sar");

	    for_init.clear();
	    init(for_init);
	}
	catch(...)
	{
	    if(reference != nullptr)
	    {
		delete reference;
		reference = nullptr;
	    }
	    throw;
	}
    }

    trivial_sar::trivial_sar(const shared_ptr<user_interaction> & dialog,
			     int filedescriptor,
			     bool lax) : generic_file(gf_read_only) , mem_ui(dialog)
    {
	label for_init;
	reference = nullptr;
	offset = 0;
	cur_pos = 0;
	end_of_slice = 0;
	hook = "";
	base = "";
	ext = "";
	old_sar = false;
	min_digits = 0;
	hook_where = "";
	base_url = "";
	natural_destruction = true;

	set_info_status(CONTEXT_INIT);
	try
	{
	    reference = new (nothrow) tuyau(dialog, filedescriptor, gf_read_only);
	    if(reference == nullptr)
		throw Ememory("trivial_sar::trivial_sar");

	    for_init.clear();
	    init(for_init);
	}
	catch(...)
	{
	    if(reference != nullptr)
	    {
		delete reference;
		reference = nullptr;
	    }
	    throw;
	}
    }

    trivial_sar::trivial_sar(const shared_ptr<user_interaction> & dialog,
			     generic_file *f,
			     const label & internal_name,
			     const label & data_name,
			     bool format_07_compatible,
			     const std::string & execute) : generic_file(gf_write_only), mem_ui(dialog)
    {
	if(f == nullptr)
	    throw SRC_BUG;

	reference = f;
	offset = 0;
	cur_pos = 0;
	end_of_slice = 0;
	hook = execute;
	base = "";
	ext = "";
	of_data_name = data_name;
	old_sar = format_07_compatible;
	min_digits = 0;
	hook_where = "";
	base_url = "";
	natural_destruction = true;

	set_info_status(CONTEXT_LAST_SLICE);
	init(internal_name);
    }

    trivial_sar::~trivial_sar()
    {
	try
	{
	    terminate();
	}
	catch(...)
	{
		/// ignore all exceptions
	}
	if(reference != nullptr)
	    delete reference;
    }

    bool trivial_sar::skip(const infinint & pos)
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(pos == cur_pos)
	    return true;
	else
	    cur_pos = pos;

	return reference->skip(pos + offset);
    }

    void trivial_sar::inherited_terminate()
    {
	if(reference != nullptr)
	{
	    char last = flag_type_terminal;

	    switch(get_mode())
	    {
	    case gf_read_only:
		break; // explicitely accepting other value
	    case gf_write_only:
	    case gf_read_write:
		if(!old_sar)
		    reference->write(&last, 1); // adding the trailing flag
		break;
	    default:
		throw SRC_BUG;
	    }

		// telling the system to free this file from the cache, when relying on a plain file
	    fichier_global *ref_fic = dynamic_cast<fichier_global *>(reference);
	    if(ref_fic != nullptr)
		ref_fic->fadvise(fichier_global::advise_dontneed);

	    reference->terminate();
	    delete reference; // this closes the slice so we can now eventually play with it:
	    reference = nullptr;
	}
	if(hook != "" && natural_destruction)
	{
	    switch(get_mode())
	    {
	    case gf_read_only:
		break;
	    case gf_write_only:
	    case gf_read_write:
		tools_hook_substitute_and_execute(get_ui(),
						  hook,
						  hook_where,
						  base,
						  "1",
						  sar_tools_make_padded_number("1", min_digits),
						  ext,
						  get_info_status(),
						  base_url);
		break;
	    default:
		throw SRC_BUG;
	    }
	}
    }

    bool trivial_sar::skip_relative(S_I x)
    {
	if(is_terminated())
	    throw SRC_BUG;

        if(x > 0)
	{
	    bool ret = reference->skip_relative(x);
            if(ret) // skip succeeded
		cur_pos += x;
	    else   // skip failed
		where_am_i();
	    return ret;
	}
	else
	{
	    U_I x_opposit = -x;
	    if(reference->get_position() > offset + x_opposit)
		return reference->skip_relative(x);
	    else
		return reference->skip(offset); // start of file

	    if(cur_pos > x_opposit)
	    {
		bool ret = reference->skip_relative(x);
		if(ret)
		    cur_pos -= x_opposit;
		else
		    where_am_i();
		return ret;
	    }
	    else
	    {
		bool ret = reference->skip(offset);
		cur_pos = 0;
		return ret;
	    }
	}
    }

    void trivial_sar::init(const label & internal_name)
    {
        header tete;

	switch(reference->get_mode())
	{
	case gf_read_only:
	    tete.read(get_ui(), *reference);
	    if(tete.get_set_flag() == flag_type_non_terminal)
		throw Erange("trivial_sar::trivial_sar", gettext("This archive has slices and is not possible to read from a pipe"));
		// if flag is flag_type_located_at_end_of_slice, we will warn at end of slice
	    offset = reference->get_position();
	    of_data_name = tete.get_set_data_name();
	    old_sar = tete.is_old_header();
	    cur_pos = 0;
	    break;
	case gf_write_only:
	case gf_read_write:
	    tete.get_set_magic() = SAUV_MAGIC_NUMBER;
	    tete.get_set_internal_name() = internal_name;
	    tete.get_set_flag() = flag_type_terminal;
	    tete.get_set_data_name() = of_data_name;
	    if(old_sar)
		tete.set_format_07_compatibility();
	    tete.write(get_ui(), *reference);
	    offset = reference->get_position();
	    cur_pos = 0;
	    break;
	default:
	    throw SRC_BUG;
	}
    }

    U_I trivial_sar::inherited_read(char *a, U_I size)
    {
	U_I ret = reference->read(a, size);
	tuyau *tmp = dynamic_cast<tuyau *>(reference);

	if(tmp != nullptr && !tmp->has_next_to_read())
	{
	    if(ret > 0)
	    {
		if(!old_sar)
		{
		    --ret;
		    if(a[ret] != flag_type_terminal)
			throw Erange("trivial_sar::inherited_read", gettext("This archive is not single sliced, more data exists in the next slices but cannot be read from the current pipe, aborting"));
		    else
			end_of_slice = 1;
		}
		else
		    end_of_slice = 1;
	    }
		// else assuming EOF has already been reached
	}

	cur_pos += ret;

	return ret;
    }

    void trivial_sar::inherited_write(const char *a, U_I size)
    {
	cur_pos += size;

	try
	{
	    reference->write(a, size);
	}
	catch(...)
	{
	    where_am_i();
	    throw;
	}
    }

    void trivial_sar::where_am_i()
    {
	cur_pos = reference->get_position();
	if(cur_pos >= offset)
	    cur_pos -= offset;
	else // we are at an invalid offset (in the slice header)
	{
	    if(!reference->skip(offset))
		throw Edata(string("trivial_sar: ")+ gettext("Cannot skip to a valid position in file"));
	    cur_pos = 0;
	}
    }

} // end of namespace

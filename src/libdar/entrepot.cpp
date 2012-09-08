//*********************************************************************/
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
// $Id: entrepot.cpp,v 1.1 2012/04/27 11:24:30 edrusb Exp $
//
/*********************************************************************/

#include "../my_config.h"

#include <typeinfo>

extern "C"
{
#if HAVE_ERRNO_H
#include <errno.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
} // end extern "C"

#include "entrepot.hpp"
#include "cygwin_adapt.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    entrepot::entrepot(): where("/"), root("/")
    {
	user = "";
	group = "";
	perm = "";
    }

    void entrepot::set_location(const path & chemin)
    {
	if(where != chemin)
	{
	    read_dir_flush();
	    where = chemin;
	}
    }

    path entrepot::get_full_path() const
    {
	if(get_location().is_relative())
	    return get_root() + get_location();
	else
	    return get_location();
    }

    entrepot::io_errors entrepot::open(user_interaction & dialog,
				       const std::string & filename,
				       gf_mode mode,
				       bool fail_if_exists,
				       bool erase,
				       hash_algo algo,
				       fichier_global * & ret) const
    {
	io_errors code;
	fichier_global *data = NULL;

	    // sanity check
	if(algo != hash_none && (mode != gf_write_only || !(erase || fail_if_exists)))
	    throw SRC_BUG;

	    // creating the file to write data to
	code = inherited_open(dialog,
			      filename,
			      mode,
			      fail_if_exists,
			      erase,
			      data);

	if(code == io_ok)
	{
	    if(data == NULL)
		throw SRC_BUG;
	    try
	    {
		if(algo != hash_none)
		{
		    fichier_global *hash_file = NULL;

			// creating the file to write hash to

		    try
		    {
			try
			{
			    code = inherited_open(dialog,
						  filename+"."+hash_algo_to_string(algo),
						  gf_write_only,
						  false,
						  true,
						  hash_file);

			    switch(code)
			    {
			    case io_ok:
				if(hash_file == NULL)
				    throw SRC_BUG;

				ret = new hash_fichier(dialog,
						       data,
						       filename,
						       hash_file,
						       algo);
				if(ret == NULL)
				    throw Ememory("entrepot::entrepot");
				else
				{
				    data = NULL;
				    hash_file = NULL;
				}
				break;
			    case io_absent:
				throw SRC_BUG; // mode is not read only or read-write
			    case io_exist:
				throw SRC_BUG; // fail_if_exist was set to false
			    default:
				throw SRC_BUG;
			    }
			}
			catch(Egeneric & e)
			{
			    e.prepend_message(gettext("Error met while creating the hash file: "));
			    throw;
			}
		    }
		    catch(...)
		    {
			if(hash_file != NULL)
			    delete hash_file;
			throw;
		    }
		}
		else
		    ret = data;
	    }
	    catch(...)
	    {
		if(data != NULL)
		{
		    delete data;
		    data = NULL;
		}
		throw;
	    }

	    return io_ok;
	}
	else
	{
	    ret = NULL;
	    return code;
	}
    }

//////////////////

    entrepot_local::entrepot_local(const std::string & perm, const std::string & user, const std::string & group, bool x_furtive_mode)
    {
	furtive_mode = x_furtive_mode;
	contents = NULL;
	set_permission(perm);
	set_user_ownership(user);
	set_group_ownership(group);
	set_root(tools_getcwd());
    }


    entrepot_local & entrepot_local::operator = (const entrepot_local & ref)
    {
	entrepot *me = this;
	const entrepot *you = &ref;

	detruit();
	*me = *you; // copying the entrepot part
	copy_from(ref); // copying the entrepot_local specific part

	return *this;
    }

    void entrepot_local::read_dir_reset()
    {
	detruit();
	user_interaction_blind aveugle;

	contents = new etage(aveugle, get_location().display().c_str(), 0, 0, false, furtive_mode);
	if(contents == NULL)
	    throw Ememory("entrepot_local::read_dir_reset");
    }

    bool entrepot_local::read_dir_next(string & filename)
    {
	if(contents == NULL)
	    return false;
	if(contents->fichier.empty())
	{
	    delete contents;
	    contents = NULL;
	    return false;
	}
	filename = contents->fichier.front();
	contents->fichier.pop_front();
	return true;
    }

    entrepot::io_errors entrepot_local::inherited_open(user_interaction & dialog,
						       const std::string & filename,
						       gf_mode mode,
						       bool fail_if_exists,
						       bool erase,
						       fichier_global * & ret) const
    {
	U_I o_mode = O_BINARY;
	int fd = -1;
	string fullname = (get_full_path() + path(filename)).display();

	switch(mode)
	{
	case gf_read_only:
	    o_mode |= O_RDONLY;
	    break;
	case gf_write_only:
	    o_mode |= O_WRONLY;
	    break;
	case gf_read_write:
	    o_mode |= O_RDWR;
	    break;
	default:
	    throw SRC_BUG;
	}

	if(mode != gf_read_only)
	{
	    o_mode |= O_CREAT;

	     if(fail_if_exists)
		 o_mode |= O_EXCL;

	     if(erase)
		 o_mode |= O_TRUNC;
	}

	try
	{
	    if(mode != gf_read_only)
		fd = ::open(fullname.c_str(), o_mode, tools_octal2int(get_permission()));
	    else
		fd = ::open(fullname.c_str(), o_mode);

	    if(fd < 0)
	    {
		switch(errno)
		{
		case EEXIST:
		    return io_exist;
		case ENOENT:
		    return io_absent;
		default:
		    throw Erange("entrepot_local::inherited_open", string(gettext("Failed openning file:")) + strerror(errno));
		}
	    }

	    if(mode != gf_read_only)
		tools_set_ownership(fd, get_user_ownership(), get_group_ownership());

	    ret = new fichier(dialog, fd);
	    if(ret == NULL)
		throw Ememory("entrepot_local::inherited_open");
	}
	catch(...)
	{
	    if(fd >= 0)
		close(fd);
	    if(ret != NULL)
	    {
		delete ret;
		ret = NULL;
	    }
	    throw;
	}

	return io_ok;
    }


    void entrepot_local::inherited_unlink(const string & filename) const
    {
	if(::unlink(filename.c_str()) != 0)
	    throw Erange("entrepot_local::inherited_unlink", tools_printf(gettext("Cannot remove file %s: "), strerror(errno)));
    }

} // end of namespace

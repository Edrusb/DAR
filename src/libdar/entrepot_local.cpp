//*********************************************************************/
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

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_ERRNO_H
#include <errno.h>
#endif

} // end extern "C"

#include "entrepot_local.hpp"
#include "cygwin_adapt.hpp"
#include "tools.hpp"
#include "fichier_local.hpp"
#include "user_interaction_blind.hpp"

using namespace std;

namespace libdar
{

    entrepot_local::entrepot_local(const std::string & user, const std::string & group, bool x_furtive_mode)
    {
	furtive_mode = x_furtive_mode;
	contents = nullptr;
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

    void entrepot_local::read_dir_reset_dirinfo() const
    {
	entrepot_local *me = const_cast<entrepot_local *>(this);
	user_interaction_blind aveugle;

	if(me == nullptr)
	    throw SRC_BUG;

	me->detruit();
	me->contents = new (nothrow) etage(aveugle, get_location().display().c_str(), datetime(0), datetime(0), false, furtive_mode);
	if(contents == nullptr)
	    throw Ememory("entrepot_local::read_dir_reset");
    }

    bool entrepot_local::read_dir_next(string & filename) const
    {
	inode_type tp;
	return read_dir_next_dirinfo(filename, tp);
    }

    bool entrepot_local::read_dir_next_dirinfo(std::string & filename, inode_type & tp) const
    {
	entrepot_local *me = const_cast<entrepot_local *>(this);

	if(me == nullptr)
	    throw SRC_BUG;
	if(contents == nullptr)
	    return false;
	if(!contents->read(filename, tp))
	{
	    delete contents;
	    me->contents = nullptr;
	    return false;
	}
	else
 	    return true;
    }

    void entrepot_local::create_dir(const std::string & dirname, U_I permission)
    {
	static const char* create_msg = "Error creating directory: ";
	static const char* owner_msg = "Error setting directory ownership: ";

	string chemin = (get_full_path() + dirname).display();

	int ret = mkdir(chemin.c_str(), permission);

	if(ret != 0)
	{
	    switch(errno)
	    {
	    case EEXIST:
		throw Esystem("entrepot_local::create_dir", gettext(create_msg) + tools_strerror_r(errno), Esystem::io_exist);
	    case ENOENT:
	    case ENOTDIR:
		throw Esystem("entrepot_local::create_dir", gettext(create_msg) + tools_strerror_r(errno), Esystem::io_absent);
	    case EACCES:
	    case EPERM:
	    case EFAULT:
		throw Esystem("entrepot_local::create_dir", gettext(create_msg) + tools_strerror_r(errno), Esystem::io_access);
	    case EROFS:
		throw Esystem("entrepot_local::create_dir", gettext(create_msg) + tools_strerror_r(errno), Esystem::io_ro_fs);
	    case ENOMEM:
		throw Ememory("entrepot_local::create_dir");
	    default:
		throw Erange("entrepot_local::create_dir", gettext(create_msg) + tools_strerror_r(errno));
	    }
	}

	uid_t uid = -1;
	uid_t gid = -1;

	if(get_user_ownership() != "")
	    uid =tools_ownership2uid(get_user_ownership());
	if(get_group_ownership() != "")
	    gid = tools_ownership2gid(get_group_ownership());

	if(uid != (uid_t)(-1) || gid != (gid_t)(-1))
	{
	    ret = chown(chemin.c_str(), uid, gid);

	    if(ret != 0)
	    {
		switch(errno)
		{
		case ENOMEM:
		    throw Ememory("entrepot_local::create_dir");
		default:
		    throw Erange("entrepot_local::create_dir", gettext(owner_msg) + tools_strerror_r(errno));
		}
	    }
	}
    }


    fichier_global *entrepot_local::inherited_open(const shared_ptr<user_interaction> & dialog,
						   const std::string & filename,
						   gf_mode mode,
						   bool force_permission,
						   U_I permission,
						   bool fail_if_exists,
						   bool erase) const
    {
	fichier_global *ret = nullptr;
	string fullname = (get_full_path().append(filename)).display();
	U_I perm = force_permission ? permission : 0666;


	ret = new (nothrow) fichier_local(dialog,
					  fullname,
					  mode,
					  perm,
					  fail_if_exists,
					  erase,
					  false);
	if(ret == nullptr)
	    throw Ememory("entrepot_local::inherited_open");
	try
	{
	    if(force_permission)
		ret->change_permission(permission); // this is necessary if the file already exists
	    if(get_user_ownership() != "" || get_group_ownership() != "")
	    {
		try
		{
		    ret->change_ownership(get_user_ownership(),
					  get_group_ownership());
		}
		catch(Ebug & e)
		{
		    throw;
		}
		catch(Egeneric & e)
		{
		    e.prepend_message("Failed setting user and/or group ownership: ");
		    throw Edata(e.get_message());
		}
	    }
	}
	catch(...)
	{
	    delete ret;
	    ret = nullptr;
	    throw;
	}

	return ret;
    }


    void entrepot_local::inherited_unlink(const string & filename) const
    {
	string target = (get_full_path().append(filename)).display();

	if(::unlink(target.c_str()) != 0)
	{
	    string err = tools_strerror_r(errno);
	    throw Erange("entrepot_local::inherited_unlink", tools_printf(gettext("Cannot remove file %s: %s"), target.c_str(), err.c_str()));
	}
    }

} // end of namespace

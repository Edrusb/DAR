//*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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

    void entrepot_local::read_dir_reset() const
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
	entrepot_local *me = const_cast<entrepot_local *>(this);

	if(me == nullptr)
	    throw SRC_BUG;
	if(contents == nullptr)
	    return false;
	if(contents->fichier.empty())
	{
	    delete contents;
	    me->contents = nullptr;
	    return false;
	}
	filename = contents->fichier.front();
	me->contents->fichier.pop_front();
	return true;
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

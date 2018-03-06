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
} // end extern "C"

#include "cat_lien.hpp"

using namespace std;

namespace libdar
{

    cat_lien::cat_lien(const infinint & uid, const infinint & gid, U_16 perm,
		       const datetime & last_access,
		       const datetime & last_modif,
		       const datetime & last_change,
		       const string & name,
		       const string & target,
		       const infinint & fs_device) : cat_inode(uid, gid, perm, last_access, last_modif, last_change, name, fs_device)
    {
	points_to = target;
	set_saved_status(saved_status::saved);
    }

    cat_lien::cat_lien(const shared_ptr<user_interaction> & dialog,
		       const smart_pointer<pile_descriptor> & pdesc,
		       const archive_version & reading_ver,
		       saved_status saved,
		       bool small) : cat_inode(dialog, pdesc, reading_ver, saved, small)
    {
	generic_file *ptr = nullptr;

	pdesc->check(small);
	if(small)
	    ptr = pdesc->esc;
	else
	    ptr = pdesc->stack;

	if(saved == saved_status::saved)
	    tools_read_string(*ptr, points_to);
    }

    bool cat_lien::operator == (const cat_entree & ref) const
    {
	const cat_lien *ref_lien = dynamic_cast<const cat_lien *>(&ref);

	if(ref_lien == nullptr)
	    return false;
	else
	    return points_to == ref_lien->points_to
		&& cat_inode::operator == (ref);
    }

    const string & cat_lien::get_target() const
    {
	if(get_saved_status() != saved_status::saved)
	    throw SRC_BUG;
	return points_to;
    }

    void cat_lien::set_target(string x)
    {
	set_saved_status(saved_status::saved);
	points_to = x;
    }

    void cat_lien::sub_compare(const cat_inode & other, bool isolated_mode) const
    {
	const cat_lien *l_other = dynamic_cast<const cat_lien *>(&other);
	if(l_other == nullptr)
	    throw SRC_BUG; // bad argument cat_inode::compare has a bug

	if(get_saved_status() == saved_status::saved && l_other->get_saved_status() == saved_status::saved)
	    if(get_target() != l_other->get_target())
		throw Erange("cat_lien:sub_compare", string(gettext("symbolic link does not point to the same target: "))
			     + get_target() + " <--> " + l_other->get_target());
    }

    void cat_lien::inherited_dump(const pile_descriptor & pdesc, bool small) const
    {
	generic_file *ptr = nullptr;

	pdesc.check(small);
	if(small)
	    ptr = pdesc.esc;
	else
	    ptr = pdesc.stack;

	cat_inode::inherited_dump(pdesc, small);

	if(get_saved_status() == saved_status::saved)
	    tools_write_string(*ptr, points_to);
    }

} // end of namespace

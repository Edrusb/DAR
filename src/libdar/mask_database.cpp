/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2026 Denis Corbin
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
} // end extern "C"

#include "mask_database.hpp"

using namespace std;

namespace libdar
{

    mask_database::mask_database(const data_dir* racine,
				 const path & fs_root,
				 const datetime & ignore_older_than_that):
	fs_racine(fs_root),
	zoom(0)
    {
	tree.reset(new (nothrow) restore_tree(racine, ignore_older_than_that));

	if(!tree)
	    throw Ememory("mask_database::mask_database");
    }

    void mask_database::compose_with(const mask & ref)
    {
	composition.reset(ref.clone());
    }

    set<archive_num> mask_database::get_locations() const
    {
	if(! tree)
	    throw SRC_BUG;
	else
	{
	    path current_path = fs_racine;
	    return tree->get_locations(current_path, composition);
	}
    }

    bool mask_database::is_covered(const std::string & expression) const
    {
	throw SRC_BUG;
    }

    bool mask_database::is_covered(const path & expression) const
    {
	    // eliminating the case where the expression is out of the requested path
	if(!composition)
	    throw SRC_BUG;
	else
	    if(!composition->is_covered(expression))
		return false;
	    // else we continue below
	    // to see if this entry should be restore
	    // from the current archive number (zoom) of the base


	    // we now have to find the corresponding data_tree
	    // from the database to know whether this entry has something
	    // to restore from the current (zoom index) archive


	path relative_part = expression;
	string tmp1, tmp2;

	if(relative_part.degre() < fs_racine.degre())
	    throw SRC_BUG;
	    // expression is shorter than fs_root, it is out of
	    // the root of the filesystem take for the operation

	if(relative_part.is_absolute() ^ fs_racine.is_absolute())
	    throw SRC_BUG;
	    // both path should be absolute
	    // or both relative

	if(relative_part.is_absolute())
	{
		// we first convert the expression to a relative path
		// by poping the front:
	    if(! relative_part.pop_front(tmp1))
	    {
		    // the expression path was "/"

		    // as fs_racine() is also absolute and has
		    // a degre() less or equal to relative_part
		    // it is also "/"

		    // in that case we should always return true
		    // as the root fs cannot be filtered out.
		return true;
	    }
	}

	fs_racine.reset_read();
	while(fs_racine.read_subdir(tmp1))
	{
	    if(! relative_part.pop_front(tmp2))
		throw SRC_BUG; // should never occur as the degree is larger or equal to fs_racine
	    if(tmp1 != tmp2)
		throw SRC_BUG; // expression is not a subpath of fs_racine
	}

	if(!tree)
	    throw SRC_BUG;
	if(zoom == 0)
	    throw SRC_BUG;

	return tree->restore_from(relative_part, zoom);
    }

} // end of namespace

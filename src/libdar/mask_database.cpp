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
} // end extern "C"

#include "mask_database.hpp"

using namespace std;

namespace libdar
{

    mask_database::mask_database(const data_dir* racine,
				 const path & fs_root,
				 const datetime & ignore_older_than_that):
	fs_racine(fs_root.display()),
	zoom(0)
    {
	tree.reset(new (nothrow) restore_tree(racine, ignore_older_than_that));

	if(!tree)
	    throw Ememory("mask_database::mask_database");
    }

    bool mask_database::is_covered(const std::string &expression) const
    {
	path relative_part = FAKE_ROOT;
	string::const_iterator eit = expression.begin();
	string::const_iterator fsit = fs_racine.begin();


	    // removing the fs_racine part of expression

	while(eit != expression.end() && fsit != fs_racine.end() && *eit == *fsit)
	{
	    ++eit;
	    ++fsit;
	}

	if(fsit != fs_racine.end())
	    throw SRC_BUG; // given expression is not a subdir of fs_racine
	else
	{
	    if(eit != expression.end())
		if(*eit != '/')
		    throw SRC_BUG; // expression starts as the given fs_root but is not a subdir of it
		else
		{
		    ++eit; // we skip over the last / to get a relative path to fs_racine
		    relative_part += path(string(eit, expression.end()));
		}
	    else  // expression equals fs_racine this the remaining part is empty
		throw SRC_BUG;
	}

	if(!tree)
	    throw SRC_BUG;
	if(zoom == 0)
	    throw SRC_BUG;

	return tree->restore_from(relative_part, zoom);
    }

} // end of namespace

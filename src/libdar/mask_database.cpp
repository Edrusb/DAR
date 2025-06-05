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
				 const datetime & ignore_older_than_that):
	zoom(0)
    {
	tree.reset(new (nothrow) restore_tree(racine, ignore_older_than_that));

	if(!tree)
	    throw Ememory("mask_database::mask_database");
    }

    bool mask_database::is_covered(const std::string &expression) const
    {
	if(!tree)
	    throw SRC_BUG;
	if(zoom == 0)
	    throw SRC_BUG;

	return tree->restore_from(expression, zoom);
    }

} // end of namespace

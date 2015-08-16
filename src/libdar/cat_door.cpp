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

#include "cat_door.hpp"
#include "null_file.hpp"

using namespace std;

namespace libdar
{

    bool cat_door::operator == (const cat_entree & ref) const
    {
	const cat_door *ref_door = dynamic_cast<const cat_door *>(&ref);

	if(ref_door == nullptr)
	    return false;
	else
	    return cat_file::operator == (ref);
    }

    generic_file *cat_door::get_data(get_data_mode mode) const
    {
	generic_file *ret = nullptr;

	if(status == from_path)
	{
	    ret = new (get_pool()) null_file(gf_read_only);
	    if(ret == nullptr)
		throw Ememory("cat_door::get_data");
	}
	else
	    ret = cat_file::get_data(mode, nullptr);

	return ret;
    }

} // end of namespace


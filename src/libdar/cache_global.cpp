/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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
}

#include "cache_global.hpp"

using namespace std;

namespace libdar
{
    cache_global::cache_global(const shared_ptr<user_interaction> & dialog, fichier_global *x_ptr, bool shift_mode, U_I size)
	: fichier_global(dialog,
			 x_ptr == nullptr ? throw SRC_BUG : x_ptr->get_mode())
    {
	ptr = x_ptr;
	buffer = new (nothrow) cache(*ptr,
				     shift_mode,
				     size);
	if(buffer == nullptr)
	    throw Ememory("cache_global::cache_global");
    }

    void cache_global::detruit()
    {
	if(buffer != nullptr)
	{
	    delete buffer;
	    buffer = nullptr;
	}
	if(ptr != nullptr)
	{
	    delete ptr;
	    ptr = nullptr;
	}
    }

} // end of namespace

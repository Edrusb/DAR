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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// to contact the author : http://dar.linux.free.fr/email.html
/*********************************************************************/

#include "../my_config.h"

extern "C"
{
} // end extern "C"

#include "pile_descriptor.hpp"

using namespace std;

namespace libdar
{

    pile_descriptor::pile_descriptor(pile *ptr)
    {
	if(ptr == nullptr)
	    throw SRC_BUG;
	stack = ptr;
	ptr->find_first_from_top(compr);
	ptr->find_first_from_bottom(esc);
    }

    void pile_descriptor::check(bool small) const
    {
	if(stack == nullptr)
	    throw SRC_BUG;
	if(esc == nullptr && small)
	    throw SRC_BUG;
	if(compr == nullptr)
	    throw SRC_BUG;
    }

} // end of namespace

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

#include "cat_storage.hpp"

using namespace std;

namespace libdar
{

    cat_storage::cat_storage(const smart_pointer<pile_descriptor> & pdesc, bool small)
    {
	if(pdesc.is_null())
	    throw SRC_BUG;
	if(small && pdesc->esc == nullptr)
	    throw SRC_BUG;

	change_location(pdesc);
    }

    void cat_storage::change_location(const smart_pointer<pile_descriptor> & x_pdesc)
    {
	if(x_pdesc.is_null())
	    throw SRC_BUG;

	if(x_pdesc->stack == nullptr)
	    throw SRC_BUG;

	if(x_pdesc->compr == nullptr)
	    throw SRC_BUG;

	pdesc = x_pdesc;
    }

    generic_file *cat_storage::get_read_cat_layer(bool small) const
    {
	generic_file *ret = nullptr;

	if(pdesc.is_null())
	    throw SRC_BUG;

	pdesc->check(small);

	if(small)
	{
	    pdesc->stack->flush_read_above(pdesc->esc);
	    ret = pdesc->esc;
	}
	else
	    ret = pdesc->stack;

	return ret;
    }

} // end of namespace

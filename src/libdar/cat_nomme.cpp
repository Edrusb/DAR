/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2021 Denis Corbin
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

#include "cat_nomme.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    cat_nomme::cat_nomme(const smart_pointer<pile_descriptor> & pdesc, bool small, saved_status val) : cat_entree(pdesc, small, val)
    {
	pdesc->check(small);
	if(small)
	    tools_read_string(*(pdesc->esc), xname);
	else
	    tools_read_string(*(pdesc->stack), xname);
    }

    bool cat_nomme::operator == (const cat_entree & ref) const
    {
	const cat_nomme *ref_nomme = dynamic_cast<const cat_nomme *>(&ref);

	if(ref_nomme == nullptr)
	    return false;
	else
	    return xname == ref_nomme->xname;
    }

    void cat_nomme::inherited_dump(const pile_descriptor & pdesc, bool small) const
    {
        cat_entree::inherited_dump(pdesc, small);

	pdesc.check(small);
	if(small)
	    tools_write_string(*pdesc.esc, xname);
	else
	    tools_write_string(*pdesc.stack, xname);
    }

} // end of namespace

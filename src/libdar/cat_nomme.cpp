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

#include "cat_nomme.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    cat_nomme::cat_nomme(const pile_descriptor & pdesc, bool small) : cat_entree(pdesc, small)
    {
	pdesc.check(small);
	if(small)
	    tools_read_string(*pdesc.esc, xname);
	else
	    tools_read_string(*pdesc.stack, xname);
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

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

#include <algorithm>

#include "cat_etoile.hpp"
#include "cat_directory.hpp"

using namespace std;

namespace libdar
{


    etoile::etoile(inode *host, const infinint & etiquette_number)
    {
        if(host == NULL)
            throw SRC_BUG;
        if(dynamic_cast<directory *>(host) != NULL)
            throw Erange("etoile::etoile", gettext("Hard links of directories are not supported"));
        hosted = host;
        etiquette = etiquette_number;
	refs.clear();
    }

    void etoile::add_ref(void *ref)
    {
	if(find(refs.begin(), refs.end(), ref) != refs.end())
	    throw SRC_BUG; // this reference is already known

	refs.push_back(ref);
    }

    void etoile::drop_ref(void *ref)
    {
	list<void *>::iterator it = find(refs.begin(), refs.end(), ref);

	if(it == refs.end())
	    throw SRC_BUG; // cannot drop a reference that does not exist

	refs.erase(it);
	if(refs.size() == 0)
	    delete this;
    }

} // end of namespace


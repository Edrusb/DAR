/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2023 Denis Corbin
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

#include <algorithm>

#include "cat_etoile.hpp"
#include "cat_directory.hpp"

using namespace std;

namespace libdar
{


    cat_etoile::cat_etoile(cat_inode *host, const infinint & etiquette_number)
    {
        if(host == nullptr)
            throw SRC_BUG;
        if(dynamic_cast<cat_directory *>(host) != nullptr)
            throw Erange("cat_etoile::cat_etoile", gettext("Hard links of directories are not supported"));
        hosted = host;
        etiquette = etiquette_number;
	refs.clear();
    }

    void cat_etoile::add_ref(void *ref)
    {
	if(find(refs.begin(), refs.end(), ref) != refs.end())
	    throw SRC_BUG; // this reference is already known

	refs.push_back(ref);
    }

    void cat_etoile::drop_ref(void *ref)
    {
	list<void *>::iterator it = find(refs.begin(), refs.end(), ref);

	if(it == refs.end())
	    throw SRC_BUG; // cannot drop a reference that does not exist

	refs.erase(it);
	if(refs.size() == 0)
	    delete this;
    }

} // end of namespace


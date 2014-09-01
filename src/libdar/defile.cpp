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
#include "defile.hpp"
#include "cat_all_entrees.hpp"

using namespace std;

namespace libdar
{

    void defile::enfile(const cat_entree *e)
    {
        const cat_eod *fin = dynamic_cast<const cat_eod *>(e);
        const cat_directory *dir = dynamic_cast<const cat_directory *>(e);
        const cat_nomme *nom = dynamic_cast<const cat_nomme *>(e);
        string s;

        if(! init) // we must remove previous entry brought by a previous call to this method
	{
            if(! chemin.pop(s))
		throw SRC_BUG; // no more cat_directory to pop!
	}
        else // nothing to be removed
            init = false;

        if(fin == NULL) // not cat_eod
	{
            if(nom == NULL) // not a cat_nomme
                throw SRC_BUG; // neither cat_eod nor cat_nomme
            else // a cat_nomme
            {
                chemin += nom->get_name();
                if(dir != NULL)
                    init = true;
            }
	}
	cache = chemin.display();
    }

} // end of namespace

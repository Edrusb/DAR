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
// to contact the author : dar.linux@free.fr
/*********************************************************************/

#include "../my_config.h"
#include "defile.hpp"

using namespace std;

namespace libdar
{

    void defile::enfile(const entree *e)
    {
        const eod *fin = dynamic_cast<const eod *>(e);
        const directory *dir = dynamic_cast<const directory *>(e);
        const nomme *nom = dynamic_cast<const nomme *>(e);
        string s;

        if(! init)
	{
            if(! chemin.pop(s))
		throw SRC_BUG; // no more directory to pop!
	}
        else
            init = false;

        if(fin == NULL)
	{
            if(nom == NULL)
                throw SRC_BUG; // neither eod nor nomme
            else
            {
                chemin += nom->get_name();
                if(dir != NULL)
                    init = true;
            }
	}
	cache = chemin.display();
    }

} // end of namespace

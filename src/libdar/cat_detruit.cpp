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

#include "cat_detruit.hpp"

using namespace std;

namespace libdar
{

    detruit::detruit(generic_file & f, const archive_version & reading_ver) : cat_nomme(f)
    {
	if(f.read((char *)&signe, 1) != 1)
	    throw Erange("detruit::detruit", gettext("missing data to build"));

	if(reading_ver > 7)
	    del_date.read(f,reading_ver);
	else
	    del_date = datetime(0);
    }

    void detruit::inherited_dump(generic_file & f, bool small) const
    {
	cat_nomme::inherited_dump(f, small);
	f.write((char *)&signe, 1);
	del_date.dump(f);
    }

} // end of namespace


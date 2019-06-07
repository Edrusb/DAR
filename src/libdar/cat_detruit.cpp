/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2019 Denis Corbin
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

#include "cat_detruit.hpp"

using namespace std;

namespace libdar
{

    cat_detruit::cat_detruit(const smart_pointer<pile_descriptor> & pdesc, const archive_version & reading_ver, bool small) : cat_nomme(pdesc, small, saved_status::saved)
    {
	generic_file *ptr = nullptr;

	pdesc->check(small);
	if(small)
	    ptr = pdesc->esc;
	else
	    ptr = pdesc->stack;

	if(ptr->read((char *)&signe, 1) != 1)
	    throw Erange("cat_detruit::cat_detruit", gettext("missing data to build"));

	if(reading_ver > 7)
	    del_date.read(*ptr, reading_ver);
	else
	    del_date = datetime(0);
    }

    bool cat_detruit::operator == (const cat_entree & ref) const
    {
	const cat_detruit *ref_det = dynamic_cast<const cat_detruit *>(&ref);

	if(ref_det == nullptr)
	    return false;
	else
	    return signe == ref_det->signe
		&& del_date == ref_det->del_date
		&& cat_nomme::operator == (ref);
    }

    void cat_detruit::inherited_dump(const pile_descriptor & pdesc, bool small) const
    {
	generic_file * ptr = nullptr;

	cat_nomme::inherited_dump(pdesc, small);

	pdesc.check(small);
	if(small)
	    ptr = pdesc.esc;
	else
	    ptr = pdesc.stack;

	ptr->write((char *)&signe, 1);
	del_date.dump(*ptr);
    }

} // end of namespace


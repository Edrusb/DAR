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
// $Id: hide_file.cpp,v 1.2.2.1 2003/05/19 20:48:06 edrusb Rel $
//
/*********************************************************************/

#include "hide_file.hpp"
#include "infinint.hpp"

#define CHECK_INIT if(!is_init) init()

hide_file::hide_file(generic_file &f) : generic_file(gf_read_only)
{
    if(f.get_mode() == gf_write_only)
	throw Erange("hide_file::hide_file", "hide_file cannot be initialized with write only file");

    ref = &f;
    if(ref == NULL)
	throw SRC_BUG; // NULL argument given
    is_init = false;
    pos_index = 0;
    pos_relicat = 0;
}

bool hide_file::skip(const infinint & pos)
{
    CHECK_INIT;
    U_I it = 0;
    while(it < morceau.size() && morceau[it].offset + morceau[it].longueur - 1 < pos)
	it++;

    if(it >= morceau.size())
	return false;
    if(morceau[it].offset > pos)
	throw SRC_BUG; // morceau has a hole in it.

    pos_index = it;
    pos_relicat = pos - morceau[it].offset;
    return true;
}

bool hide_file::skip_to_eof()
{
    CHECK_INIT;
    pos_index = morceau.size();
    return true;
}

bool hide_file::skip_relative(S_I x)
{
    CHECK_INIT;
    if(x > 0)
    {
	infinint my_x = x;
	while(my_x > 0 && pos_index < morceau.size())
	{
	    if(pos_relicat + my_x >= morceau[pos_index].longueur)
	    {
		my_x -= morceau[pos_index].longueur - pos_relicat;
		pos_relicat = 0;
		pos_index++;
	    }
	    else
	    {
		pos_relicat += my_x;
		my_x = 0;
	    }
	}

	return pos_index < morceau.size();
    }
    else if(x < 0)
    {
	infinint my_x = -x; // make x a positive value
	while(my_x > 0 && (pos_index < morceau.size() || pos_relicat > 0))
	{
	    if(my_x > pos_relicat)
	    {
		pos_index--;
		if(pos_index < morceau.size())
		{
		    my_x -= pos_relicat;
		    pos_relicat = morceau[pos_index].longueur;
		}
		else
		{
		    pos_index = 0;
		    return false;
		}
	    }
	    else
		pos_relicat -= my_x;
	    return true;
	}
    }
    return true;
}

infinint hide_file::get_position()
{
    CHECK_INIT;
    if(pos_index >= morceau.size())
	return morceau.back().offset + morceau.back().longueur;
    else
	return morceau[pos_index].offset + pos_relicat;
}

S_I hide_file::inherited_read(char *a, size_t size)
{
    CHECK_INIT;
    U_I lu = 0;
    size_t maxlire;
    size_t reste;

    while(lu < size && pos_index < morceau.size())
    {
	maxlire = 0;
	(morceau[pos_index].longueur - pos_relicat).unstack(maxlire);
	reste = size - lu;
	maxlire = maxlire > reste ? reste : maxlire;

	ref->skip(morceau[pos_index].debut+pos_relicat);
	lu += ref->read(a+lu, maxlire);
	pos_relicat += lu;
	if(pos_relicat >= morceau[pos_index].longueur)
	{
	    pos_index++;
	    pos_relicat = 0;
	}
    }

    return lu;
}

static void dummy_call(char *x)
{
    static char id[]="$Id: hide_file.cpp,v 1.2.2.1 2003/05/19 20:48:06 edrusb Rel $";
    dummy_call(id);
}

S_I hide_file::inherited_write(const char *a, size_t size)
{
    CHECK_INIT;
    throw SRC_BUG; // hide_file alsways is read-only !
}


void hide_file::init()
{
    fill_morceau();
    is_init = true;
}

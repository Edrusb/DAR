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
#include "hide_file.hpp"

using namespace libdar;

#define CHECK_INIT if(!is_init) init()

hide_file::hide_file(generic_file &f) : generic_file(gf_read_only)
{
    if(f.get_mode() == gf_write_only)
        throw Erange("hide_file::hide_file", gettext("hide_file cannot be initialized with write-only file"));

    ref = &f;
    if(ref == nullptr)
        throw SRC_BUG; // nullptr argument given
    is_init = false;
    pos_index = 0;
    pos_relicat = 0;
}

bool hide_file::skip(const infinint & pos)
{
    CHECK_INIT;
    if(is_terminated())
	throw SRC_BUG;

    U_I it = 0;
    while(it < morceau.size() && morceau[it].offset + morceau[it].longueur - 1 < pos)
        ++it;

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
    if(is_terminated())
	throw SRC_BUG;

    pos_index = morceau.size();
    return true;
}

bool hide_file::skip_relative(S_I x)
{
    CHECK_INIT;
    if(is_terminated())
	throw SRC_BUG;

    if(x > 0)
    {
        infinint my_x = x;
        while(my_x > 0 && pos_index < morceau.size())
        {
            if(pos_relicat + my_x >= morceau[pos_index].longueur)
            {
                my_x -= morceau[pos_index].longueur - pos_relicat;
                pos_relicat = 0;
                ++pos_index;
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
                --pos_index;
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

infinint hide_file::get_position() const
{
    CHECK_INIT;
    if(is_terminated())
	throw SRC_BUG;

    if(pos_index >= morceau.size())
	if(morceau.empty())
	    return 0; // empty virtual file
	else
	    return morceau.back().offset + morceau.back().longueur;
    else
        return morceau[pos_index].offset + pos_relicat;
}

U_I hide_file::inherited_read(char *a, U_I size)
{
    CHECK_INIT;
    if(is_terminated())
	throw SRC_BUG;

    U_I lu = 0, tmp_lu = 0;
    size_t maxlire;
    size_t reste;

    while(lu < size && pos_index < morceau.size())
    {
        maxlire = 0;
        (morceau[pos_index].longueur - pos_relicat).unstack(maxlire);
        reste = size - lu;
        maxlire = maxlire > reste ? reste : maxlire;

        ref->skip(morceau[pos_index].debut+pos_relicat);
        tmp_lu = ref->read(a+lu, maxlire);
	if(tmp_lu == 0 && maxlire > 0)
	    throw SRC_BUG;
	lu += tmp_lu;
        pos_relicat += lu;
        if(pos_relicat >= morceau[pos_index].longueur)
        {
            pos_index++;
            pos_relicat = 0;
        }
    }

    return lu;
}

void hide_file::inherited_write(const char *a, size_t size)
{
    CHECK_INIT;
    throw SRC_BUG; // hide_file alsways is read-only !
}

void hide_file::init() const
{
    hide_file *me = const_cast<hide_file *>(this);

    if(me == nullptr)
	throw SRC_BUG;
    me->fill_morceau();
    me->is_init = true;
}

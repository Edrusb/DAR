/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2018 Denis Corbin
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

#include "secu_memory_file.hpp"

using namespace std;

namespace libdar
{
    bool secu_memory_file::skip(const infinint & pos)
    {
	infinint tmp = pos;

	if(is_terminated())
	    throw SRC_BUG;

	if(tmp >= data.get_size())
	{
	    position = data.get_size();
	    return false;
	}
	else
	{
	    position = 0;
	    tmp.unstack(position);
	    if(!tmp.is_zero())
		throw SRC_BUG; // pos < data.size(), which is typed U_I as well as position
	    return true;
	}
    }

    bool secu_memory_file::skip_to_eof()
    {
	if(is_terminated())
	    throw SRC_BUG;

	position = data.get_size();
	return true;
    }

    bool secu_memory_file::skip_relative(S_I x)
    {
	bool ret = false;

	if(is_terminated())
	    throw SRC_BUG;

	if(x < 0)
	{
	    U_I tx = -x;
	    if(position < tx)
	    {
		position = 0;
		ret = false;
	    }
	    else
	    {
		position -= tx;
		ret = true;
	    }
	}
	else
	{
	    position += x;
	    if(position > data.get_size())
	    {
		position = data.get_size();
		ret = false;
	    }
	    else
		ret = true;
	}

	return ret;
    }

    U_I secu_memory_file::inherited_read(char *a, U_I size)
    {
	U_I lu = 0;
	const char *deb = data.c_str() + position;

	while(lu < size && position + lu < data.get_size())
	{
	    *a = *deb;
	    ++lu;
	    ++a;
	    ++deb;
	}
	position += lu;

	return lu;
    }

    void secu_memory_file::inherited_write(const char *a, U_I size)
    {
	data.append_at(position, a, size);
	position += size;
    }

}  // end of namespace

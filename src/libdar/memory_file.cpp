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

#include "memory_file.hpp"

using namespace std;

namespace libdar
{
    bool memory_file::skip(const infinint & pos)
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(pos >= data.size())
	{
	    position = data.size();
	    return false;
	}
	else
	{
	    position = pos;
	    return true;
	}
    }

    bool memory_file::skip_to_eof()
    {
	if(is_terminated())
	    throw SRC_BUG;

	position = data.size();
	return true;
    }

    bool memory_file::skip_relative(S_I x)
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
	    if(position > data.size())
	    {
		position = data.size();
		ret = false;
	    }
	    else
		ret = true;
	}

	return ret;
    }

    U_I memory_file::inherited_read(char *a, U_I size)
    {
	U_I ret = 0;

	while(ret < size && position < data.size())
	{
	    *(a++) = (char)(data[position]);
	    ++ret;
	    ++position;
	}

	return ret;
    }

    void memory_file::inherited_write(const char *a, U_I size)
    {
	U_I ret = 0;

	if(size == 0)
	    return;

	while(ret < size && position < data.size())
	{
	    data[position] = (unsigned char)(*(a++));
	    ret++;
	    ++position;
	}

	data.insert_bytes_at_iterator(data.end(), (unsigned char *)(a), size - ret);
	position += size - ret;
    }

}  // end of namespace

/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2026 Denis Corbin
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

#include "memory_file.hpp"

using namespace std;

namespace libdar
{



    bool memory_file::skip(const infinint & pos)
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(pos >= data_size)
	{
	    position = data_size;
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

	position = data_size;
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
	    if(position > data_size)
	    {
		position = data_size;
		ret = false;
	    }
	    else
		ret = true;
	}

	return ret;
    }

    infinint memory_file::dump_to(unsigned char* a, const infinint & size) const
    {
	if(data_size > size)
	    return size + 1;
	else
	{
	    storage::iterator it = data.begin();
	    unsigned char *ptr = a;

	    while(it != data.end())
	    {
		*ptr = *it;
		++ptr;
		++it;
	    }

	    return data_size;
	}
    }

    void memory_file::load_from(unsigned char* a, const infinint & size)
    {
	storage::iterator it;
	U_32 step;
	unsigned char* ptr = a;
	infinint to_xfer = size;

	    // expanding the storage if needed

	if(data_size < size)
	{
	    infinint to_add = size - data_size;
	    U_I step;

	    while(!to_add.is_zero())
	    {
		step = 0;
		to_add.unstack(step);
		data.insert_null_bytes_at_iterator(data.end(), step);
	    }
	}

	    // shrinking the storage if needed

	if(data_size > size)
	    data.truncate(size);

	data_size = size;

	it = data.begin();

	while(!to_xfer.is_zero())
	{
	    step = 0;
	    to_xfer.unstack(step);
	    data.write(it, ptr, step);
	    it  += step;
	    ptr += step;
	}

	position = 0;
    }

    U_I memory_file::inherited_read(char *a, U_I size)
    {
	U_I ret = 0;

	while(ret < size && position < data_size)
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

	while(ret < size && position < data_size)
	{
	    data[position] = (unsigned char)(*(a++));
	    ret++;
	    ++position;
	}

	size -= ret; // size is now the amount of extra bytes needed to be added at then end of storage

	data.insert_bytes_at_iterator(data.end(), (unsigned char *)(a), size);
	position += size; // position has been increased up to size

	if(position > data_size) // storage has been extended
	    data_size = position;

    }

    void memory_file::inherited_truncate(const infinint & pos)
    {
	if(pos >= data_size)
	    throw SRC_BUG; // truncating after the last byte !?!
	data.truncate(pos);
	if(position > pos)
	    position = pos;
	data_size = pos;
    }

}  // end of namespace

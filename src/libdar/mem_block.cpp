/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2025 Denis Corbin
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
#ifdef HAVE_STRING_H
#include <string.h>
#endif
}

#include "mem_block.hpp"
#include "erreurs.hpp"

using namespace std;

namespace libdar
{

    mem_block::mem_block(U_I size)
    {
	data = nullptr;
	resize(size);
    }

    mem_block::mem_block(mem_block && ref) noexcept
    {
	try
	{
	    data = nullptr;
	    move_from(std::move(ref));
	}
	catch(...)
	{
		// noexcept
	}
    }

    mem_block::~mem_block()
    {
	if(data != nullptr)
	    delete [] data;
    }

    mem_block & mem_block::operator = (mem_block && ref) noexcept
    {
	try
	{
	    move_from(std::move(ref));
	}
	catch(...)
	{
		// noexcept
	}

	return *this;
    }

    void mem_block::resize(U_I size)
    {
	if(data != nullptr)
	{
	    delete [] data;
	    data = nullptr;
	}

	if(size > 0)
	{
	    data = new (nothrow) char[size];
	    if(data == nullptr)
		throw Ememory("mem_block::mem_block");
	}
	alloc_size = size;
	data_size = 0;
	read_cursor = 0;
	write_cursor = 0;
    }

    U_I mem_block::read(char *a, U_I size)
    {
	if(data_size < read_cursor)
	    throw SRC_BUG;
	else
	{
	    U_I remains = data_size - read_cursor;
	    U_I amount = size < remains ? size : remains;

	    memcpy(a, data + read_cursor, amount);
	    read_cursor += amount;

	    return amount;
	}
    }

    U_I mem_block::write(const char *a, U_I size)
    {
	if(alloc_size < write_cursor)
	    throw SRC_BUG;
	else
	{
	    U_I remains = alloc_size - write_cursor;
	    U_I amount = size < remains ? size : remains;

	    memcpy(data + write_cursor, a, amount);
	    write_cursor += amount;
	    if(data_size < write_cursor)
		data_size = write_cursor;

	    return amount;
	}
    }

    void mem_block::rewind_read(U_I offset)
    {
	if(offset > data_size)
	    throw Erange("mem_block::reset_read", "offset out of range for mem_block");
	read_cursor = offset;
    }

    void mem_block::set_data_size(U_I size)
    {
	if(size > alloc_size)
	    throw SRC_BUG;
	data_size = size;
	if(read_cursor < size)
	    read_cursor = size;
	if(write_cursor < size)
	    write_cursor = size;
    }

    void mem_block::move_from(mem_block && ref)
    {
	swap(data, ref.data);
	alloc_size = std::move(ref.alloc_size);
	data_size = std::move(ref.data_size);
	read_cursor = std::move(ref.read_cursor);
	write_cursor = std::move(ref.write_cursor);
    }

} // end of namespace

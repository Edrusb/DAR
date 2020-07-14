/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2020 Denis Corbin
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

    mem_block::~mem_block()
    {
	if(data != nullptr)
	    delete [] data;
    }

    void mem_block::resize(U_I size)
    {
	if(data != nullptr)
	    delete [] data;
	data = nullptr;

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
	U_I remains = data_size - read_cursor;
	U_I amount = size < remains ? size : remains;

	memcpy(a, data, amount);
	read_cursor += amount;

	return amount;
    }

    U_I mem_block::write(const char *a, U_I size)
    {
	U_I remains = alloc_size - write_cursor;
	U_I amount = size < remains ? size : remains;

	memcpy(data + write_cursor, a, amount);
	write_cursor += amount;
	data_size += amount;

	return amount;
    }

    void mem_block::rewind_read(U_I offset)
    {
	if(offset > data_size)
	    throw Erange("mem_block::reset_read", "offset out of range for mem_block");
	read_cursor = offset;
    }

} // end of namespace

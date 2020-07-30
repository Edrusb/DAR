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

    /// \file mem_block.hpp
    /// \brief structure to hold block of memory and manipulate in coherence with idexes and sizes
    /// \ingroup Private
    ///

#ifndef MEM_BLOCK_HPP
#define MEM_BLOCK_HPP

#include "../my_config.h"
#include <string>

#include "integers.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

    class mem_block
    {
    public:
	mem_block(U_I size = 0);
	mem_block(const mem_block & ref) = delete;
	mem_block(mem_block && ref) noexcept;
	mem_block & operator = (const mem_block & ref) = delete;
	mem_block & operator = (mem_block && ref) noexcept;
	virtual ~mem_block();


	void resize(U_I size);

	U_I read(char *a, U_I size);            ///< read data from the mem_block, returns the amount read
	U_I write(const char *a, U_I size);     ///< write data to the mem_block, returns the amount wrote
	void rewind_read(U_I offset = 0); ///< reset read cursor
	void reset() { data_size = 0; read_cursor = 0; write_cursor = 0; };
	U_I get_max_size() const { return alloc_size; };
	U_I get_data_size() const { return data_size; };
	U_I get_read_offset() const { return read_cursor; };
	bool all_is_read() const { return read_cursor == data_size; };
	bool is_full() const { return data_size == alloc_size; };
	bool is_empty() const { return data_size == 0; };

	char* get_addr() { return data; };
	void set_data_size(U_I size);

    private:
	char* data;
	U_I alloc_size;
	U_I data_size;
	U_I read_cursor;
	U_I write_cursor;

	void move_from(mem_block && ref);
    };


	/// @}

} // end of namespace

#endif

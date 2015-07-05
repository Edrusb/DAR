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

#include "mem_cluster.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    mem_cluster::mem_cluster(U_I x_block_size, U_I table_size_64, mem_manager *x_holder) : mem_allocator(x_holder)
    {
	block_size = x_block_size > 0 ? x_block_size : 1; // trivial case of zero handled by 1 byte length pointed areas
	alloc_table_size = table_size_64;
	next_free_in_table = 0;
	max_available_blocks = table_size_64 * 64;
	available_blocks = max_available_blocks;
	alloc_area_size = max_available_blocks * block_size;
	alloc_table = nullptr;
	alloc_area = nullptr;
#ifdef LIBDAR_DEBUG_MEMORY
	min_avail_reached = max_available_blocks;
#endif

	try
	{
	    alloc_table = (U_64 *)new (nothrow) char[alloc_table_size*sizeof(U_64) + alloc_area_size];
	    if(alloc_table == nullptr)
		throw Ememory("mem_cluster::mem_cluster");
	    alloc_area = (char *)(alloc_table + alloc_table_size);

	    for(U_I i = 0; i < alloc_table_size; ++i)
		alloc_table[i] = 0;
	}
	catch(...)
	{
	    if(alloc_table != nullptr)
		delete [] alloc_table;
	    throw;
	}
    }

    mem_cluster::~mem_cluster()
    {
	if(alloc_table != nullptr)
	    delete [] alloc_table;
    }

    void *mem_cluster::alloc()
    {
	void * ret = nullptr;

	if(is_full())
	    throw SRC_BUG;

	while(next_free_in_table < alloc_table_size && alloc_table[next_free_in_table] == FULL)
	    ++next_free_in_table;

	if(next_free_in_table == alloc_table_size)
	{
	    next_free_in_table = 0;

	    while(next_free_in_table < alloc_table_size && alloc_table[next_free_in_table] == FULL)
		++next_free_in_table;

	    if(next_free_in_table == alloc_table_size)
		throw SRC_BUG; // should be reported as full by full() method
	}

	U_I offset = find_free_slot_in(next_free_in_table);
	ret = alloc_area + block_size * (next_free_in_table * 64 + offset);
	set_slot_in(next_free_in_table, offset, true);
	--available_blocks;
#ifdef LIBDAR_DEBUG_MEMORY
	if(available_blocks < min_avail_reached)
	    min_avail_reached = available_blocks;
#endif

	return ret;
    }

    U_I mem_cluster::find_free_slot_in(U_I table_integer) const
    {
	U_I ret = 0;
	U_64 focus = alloc_table[table_integer]; // we copy to be able to modify


	while(focus > HALF)
	{
		// the most significant bit is 1 so the corresponding block is already allocated
		// looking at the next offset
	    focus <<= 1;
	    ++ret;
	}

	return ret;
    }

    void mem_cluster::set_slot_in(U_I table_integer, U_I bit_offset, bool value)
    {
	U_64 add_mask = LEAD >> bit_offset;

	if(value)
	{
		// we must first check the block is not already allocated
	    if((alloc_table[table_integer] & add_mask) != 0)
		throw SRC_BUG; // double allocation
		// OK, so we can mark that block as allocated
	    alloc_table[table_integer] |= add_mask;
	}
	else
	{
		// we must first check the block is not already released
	    if((alloc_table[table_integer] & add_mask) == 0)
		throw SRC_BUG; // double release
		// OK, si we can mark that block as released
	    alloc_table[table_integer] &= ~add_mask;
	}
    }

    void mem_cluster::release(void *ptr)
    {
	if(ptr < alloc_area || ptr >= alloc_area + alloc_area_size)
	    throw SRC_BUG; // not allocated here
	else
	{
	    U_I char_offset = (char *)(ptr) - alloc_area;
	    U_I block_number = char_offset / block_size;
	    U_I table_integer = block_number / 64;
	    U_I offset = block_number % 64;

	    if(char_offset % block_size != 0)
		throw SRC_BUG; // not at a block boundary

	    set_slot_in(table_integer, offset, false);
	    ++available_blocks;
	    if(available_blocks > max_available_blocks)
		throw SRC_BUG;
	    if(is_empty())
		get_manager().push_to_release_list(this);
	}
    }

    string mem_cluster::dump() const
    {
	string ret = "";
	U_I counted = max_available_blocks - available_blocks;

	ret += "      Cluster dump:\n";
	ret += tools_printf("         block_size            = %d\n", block_size);
	ret += tools_printf("         available_blocks      = %d\n", available_blocks);
	ret += tools_printf("         max_available_blocks  = %d\n", max_available_blocks);
	ret += tools_printf("         which makes %d unreleased block(s)\n", counted);
	ret += tools_printf("         Follows the list of unreleased blocks for that cluster:\n");
	ret += examination_status();
	ret += "\n\n";

	return ret;
    }

    string mem_cluster::examination_status() const
    {
	string ret = "";

	for(U_I table_ptr = 0; table_ptr < alloc_table_size; ++table_ptr)
	{
	    U_64 mask = LEAD;

	    for(U_I offset = 0; offset < 64; ++offset)
	    {
		if((alloc_table[table_ptr] & mask) != 0)
		    ret += tools_printf("                 unreleased memory (%d bytes) at: 0x%x\n",
					block_size,
					(U_I)(alloc_area + block_size * ( 64 * table_ptr + offset)));
		mask >>= 1;
	    }
	}

	return ret;
    }

}  // end of namespace

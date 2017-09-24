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

    /// \file mem_cluster.hpp
    /// \brief defines mem_cluster class that holds a fixed set of fixed size allocatable memory blocks
    /// \ingroup Private



#ifndef MEM_CLUSTER_HPP
#define MEM_CLUSTER_HPP

#include "../my_config.h"
#include "mem_allocator.hpp"


namespace libdar
{


	/// \addtogroup Private
	/// @{

    class mem_cluster : public mem_allocator
    {
    public:
	mem_cluster(U_I x_block_size,   //< block size that will be allocated from this mem_cluster
		    U_I table_size_64,      //< the total number of block in this mem_cluster is table_size_64 * 64
		    mem_manager *x_holder); //< this is the object that holds this mem_cluster object
	mem_cluster(const mem_cluster & ref) = delete;
	mem_cluster & operator = (const mem_cluster & ref) = delete;
	~mem_cluster();

	    /// return true if all available memory blocks have been allocated
	bool is_full() const { return available_blocks == 0; };

	    /// return true if non of the allocated memory block have been allocated
	bool is_empty() const { return available_blocks == max_available_blocks; };

	    /// returns a pointer to a newly allocated memory block of the size given at construction time
	void *alloc();

	    /// returns the block size used at construction time
	U_I get_block_size() const { return block_size; };

	    /// provides a status of the current object
	std::string dump() const;

	    /// inherited from allocator, allow an allocated memory block to be recycled as available memory block
	virtual void release(void *ptr);

#ifdef LIBDAR_DEBUG_MEMORY
	virtual U_I max_percent_full() const { return (max_available_blocks - min_avail_reached)*100/max_available_blocks; };
#else
	virtual U_I max_percent_full() const { return 0; };
#endif

    private:
	static const U_64 FULL = ~(U_64)(0);            //< this is 1111...111 integer in binary notation
	static const U_64 HALF = (~(U_64)(0)) >> 1;     //< this is 0111...111 integer in binary notation
	static const U_64 LEAD = ~((~(U_64)(0)) >> 1);  //< this is 1000...000 integer in binary notation

	    // the memory obtained by that object is split in two parts:
	    // - the alloc_table which tells what block is sub-allocated or not
	    // - the alloc_area which contains all the blocks that can be sub-allocated
	    // all this memory is obtained at once and the address to release at object destructor is given by alloc_table
	    // because it takes place at the beginning of the obtained memory
	char *alloc_area;          //< points to the allocatable memory block
	U_I alloc_area_size;       //< size of sub-allocatable memory in bytes (excluding the alloc_table part of the allocated memory used for management)
	U_I block_size;            //< size of requested blocks
	U_64 *alloc_table;         //< maps the blocks of the allocated memory that have been (sub-)allocated
	U_I alloc_table_size;      //< size of the map (in number of U_64 integers)
	U_I next_free_in_table;    //< next U_64 to look at for a request of block allocation
	U_I available_blocks;      //< current number of available block in alloc
	U_I max_available_blocks;  //< max available block in alloc
#ifdef LIBDAR_DEBUG_MEMORY
	U_I min_avail_reached;     //< records the max fullfilment reached
#endif

	U_I find_free_slot_in(U_I table_integer) const;
	void set_slot_in(U_I table_integer, U_I bit_offset, bool value);
	std::string examination_status() const; // debugging, displays the list of allocated blocks that remain
    };

	/// @}

} // end of namespace

#endif

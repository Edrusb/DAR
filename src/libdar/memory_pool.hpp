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

    /// \file memory_pool.hpp
    /// \brief class memory_pool allocates and recycles blocks of memory for better performances
    /// it is expected to be used with classes that inherit from class on_pool
    /// \ingroup API


#ifndef MEMORY_POOL_HPP
#define MEMORY_POOL_HPP

#include "../my_config.h"
#include <map>
#include "mem_sized.hpp"


namespace libdar
{
	/// \addtogroup API
	/// @{

    class memory_pool
    {
    public:
#ifdef LIBDAR_SPECIAL_ALLOC
	memory_pool() { carte.clear(); };
#else
	memory_pool() { throw Efeature("Special allocation"); };
#endif
	memory_pool(const memory_pool & ref) { throw SRC_BUG ; };
	const memory_pool & operator = (const memory_pool & ref) { throw SRC_BUG; };
	~memory_pool();


	    /// allocate a memory block of requested size
	    ///
	    /// \param[in] size the size of the requested memory block to allocate
	    /// \param[ou] ptr the mem_allocator object that will have to be informed when that memory block will be released
	    /// \return the address of the newly allocated memory block
	void *alloc(size_t size);

	    /// release a memory block previously allocated by the memory pool
	void release(void *ptr);

	    /// cleanup mem_sized objects that were pending for release
	void garbage_collect();

	    /// return true if all memory blocks previously allocated have been released
	bool is_empty() const { return carte.size() == 0; };

	    /// display a memory_pool status
	std::string dump() const;

#ifdef LIBDAR_DEBUG_MEMORY

	    /// provides information about the usage of the memory_pool
	std::string max_percent_full() const;
#endif

    private:

	    /// this data structure is placed just before an allocated block to provide a quick way to release it
	union alloc_ptr
	{
	    mem_allocator *ptr;//< points to the mem_allocator that provided the block
	    U_I  alignment__i; //< to properly align the allocated memory block that follows
	    U_32 alignment_32; //< to properly align the allocated memory block that follows
	    U_64 alignment_64; //< to properly align the allocated memory block that follows
	};

	std::map<U_I, mem_sized *> carte; //< associate a requested block size to the corresponding mem_sized object
#ifdef LIBDAR_DEBUG_MEMORY
	std::map<U_I, U_I> count;         //< counts the usage of requests per requested block size
#endif
    };

	/// @}

} // end of namespace

#endif

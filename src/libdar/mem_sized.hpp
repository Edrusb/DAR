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

    /// \file mem_sized.hpp
    /// \brief defines mem_sized class that holds a variable sized set of fixed sized blocks using class mem_cluster
    /// \ingroup Private



#ifndef MEM_SIZED_HPP
#define MEM_SIZED_HPP

#include "../my_config.h"
#include "mem_allocator.hpp"
#include "mem_cluster.hpp"
#include <list>

namespace libdar
{


	/// \addtogroup Private
	/// @{

    class mem_sized : public mem_manager
    {
    public:
	mem_sized(U_I x_block_size);
	mem_sized(const mem_sized & ref) = delete;
	mem_sized & operator = (const mem_sized & ref) = delete;
	~mem_sized();

	    /// to allocate a block of memory
	    ///
	    /// \param[out] ptr is set to the address of the mem_allocator that will have to be informed
	    /// when the returned block will be released
	    /// \return the address of the newly allocated block or nullptr upon failure
	void *alloc(mem_allocator * & ptr);

	    /// returns true if all memory block so far allocated have been released
	bool is_empty() const;

	    /// display a status of current mem_sized
	std::string dump() const;

#ifdef LIBDAR_DEBUG_MEMORY

	    /// provides information about the usage of the memory_pool
	U_I max_percent_full() const;
#endif

	    /// inherited from mem_manager
	virtual void push_to_release_list(mem_allocator *ref);

    private:
	static const U_I average_table_size = 10240;

	U_I table_size_64;                                     //< size for clusters
	std::list<mem_cluster *> clusters;                     //< the list of owned mem_cluster objects
	std::list<mem_cluster *>::iterator next_free_in_table; //< points to an mem_cluster that is known to have free slots
	mem_cluster *pending_release;                          //< a totally free cluster that can be recycled if all others are full and a new block is requested or freed if another mem_cluster becomes also totally free
#ifdef LIBDAR_DEBUG_MEMORY
	U_I sum_percent;   //< summ of all max percent usage obtained before deleting a mem_cluster object (used for statistical purposes)
	U_I num_cluster;   //< number of cluster which sum_percent has been collected from (sum_percent/num_cluster gives the average load of mem_clusters)
#endif
    };

	/// @}

} // end of namespace

#endif

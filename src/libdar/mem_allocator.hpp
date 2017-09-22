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

    /// \file mem_allocator.hpp
    /// \brief this is the base class of object that can provide dynamically allocated memory blocks
    /// \ingroup Private


#ifndef MEM_ALLOCATOR_HPP
#define MEM_ALLOCATOR_HPP

#include "../my_config.h"
#include "erreurs.hpp"

namespace libdar
{


	/// \addtogroup Private
	/// @{

	/// forward declaration
    class mem_manager;

	/// generic interface of objects that need to be informed that a memory block they manage has been released

    class mem_allocator
    {
    public:
	mem_allocator(mem_manager *ptr) { if(ptr == nullptr) throw SRC_BUG; manager = ptr; };
	mem_allocator(const mem_allocator & ref) { throw SRC_BUG; };
	mem_allocator & operator = (const mem_allocator & ref) { throw SRC_BUG; };
	virtual ~mem_allocator() {};

	    /// this is the interface to use to release a memory block owned by this mem_allocator
	virtual void release(void *ptr) = 0;

	    /// returns the maximum occupation reached for that object (used for debugging purposes)
	virtual U_I max_percent_full() const = 0;

    protected:
	mem_manager & get_manager() { return *manager; };

    private:
	mem_manager *manager;
    };



	/// generic interface of memory managers that create and delete mem_allocator objects depending on requests

    class mem_manager
    {
    public:
	virtual ~mem_manager() {};

	    /// this is for the mem_allocator to inform its mem_manager that it has all its block released
	virtual void push_to_release_list(mem_allocator *ref) = 0;
    };


	/// @}

} // end of namespace

#endif

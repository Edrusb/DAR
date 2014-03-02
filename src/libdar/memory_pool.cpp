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

#include "memory_pool.hpp"
#include "memory_check.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

    memory_pool::~memory_pool()
    {
	map<U_I, mem_sized *>::iterator it = carte.begin();

	while(it != carte.end())
	{
	    if(it->second != NULL)
	    {
		if(!it->second->is_empty())
		    throw SRC_BUG;
		delete it->second;
		it->second = NULL;
	    }
	    ++it;
	}
    }

    void *memory_pool::alloc(size_t size)
    {
	alloc_ptr *ret = NULL;      //< address that will be returned
	mem_allocator *ptr = NULL;  //< address of the object that has to be informed of memory release for the allocated block
	size_t with_overhead = size + sizeof(alloc_ptr);
	map<U_I, mem_sized *>::iterator it = carte.find(with_overhead);

#ifdef LIBDAR_DEBUG_MEMORY

	    // updating statistics

	map<U_I, U_I>::iterator cit = count.find(with_overhead);
	if(cit == count.end())
	    count[size] = 1;
	else
	    ++(cit->second);
#endif

	    // looking for a existing mem_sized object

	if(it != carte.end())
	    if(it->second == NULL)
		throw SRC_BUG;
	    else  // found an existing mem_sized, requesting it a memory block
		ret = (alloc_ptr *)it->second->alloc(ptr);
	else
	{
		// we must first create a new mem_sized

	    memory_check_special_new_sized(with_overhead);
	    mem_sized *tmp = new (nothrow) mem_sized(with_overhead);
	    if(tmp == NULL)
		throw SRC_BUG;

		// we record the brand-new mem_sized
	    try
	    {
		carte[with_overhead] = tmp;
	    }
	    catch(...)
	    {
		delete tmp;
		throw;
	    }

		// and request it a memory block

	    ret = (alloc_ptr *)tmp->alloc(ptr);
	}

	    // filling the overhead structure before the memory block

	if(ret != NULL)
	{
	    try
	    {
		if(ptr == NULL)
		    throw SRC_BUG;
		ret->ptr = ptr;
		++ret;
		memory_check_special_report_new(ret, size);
	    }
	    catch(...)
	    {
		delete ret;
		throw;
	    }
	}

	return (void *)ret;
    }

    void memory_pool::release(void *ptr)
    {
	alloc_ptr *tmp = (alloc_ptr *)ptr;

	if(tmp == NULL)
	    throw SRC_BUG; // trying to release block at NULL
	--tmp;  // moving to the previous header
	if(tmp->ptr == NULL)
	    throw SRC_BUG; // NULL found for the mem_allocator of that block
	tmp->ptr->release((void *)tmp);
	memory_check_special_report_delete(ptr);
    }

    void memory_pool::garbage_collect()
    {
	map<U_I, mem_sized *>::iterator it = carte.begin();

	while(it != carte.end())
	{
	    if(it->second == NULL)
		throw SRC_BUG;
	    if(it->second->is_empty())
	    {
		map<U_I, mem_sized *>::iterator tmp = it;

		++it;
		delete tmp->second;
		carte.erase(tmp);
	    }
	    else
		++it;
	}
#ifdef LIBDAR_DEBUG_MEMORY
	count.clear();
#endif
    }

    string memory_pool::dump() const
    {
	string ret = "";
	map<U_I, mem_sized *>::const_iterator it = carte.begin();

	ret += "###################################################################\n";
	ret += "  SPECIAL ALLOCATION MODULE REPORTS UNRELEASED MEMORY ALLOCATIONS\n\n";
	while(it != carte.end())
	{
	    if(it->second == NULL)
		ret += tools_printf("!?! NO corresponding mem_sized object for block size %d\n", it->first);
	    else
	    {
		if(!it->second->is_empty())
		{
		    ret += tools_printf("Dumping list for blocks of %d bytes size", it->first);
		    ret += it->second->dump();
		}
	    }
	    ++it;
	}
	ret += "###################################################################\n";

	return ret;
    }


#ifdef LIBDAR_DEBUG_MEMORY
    string memory_pool:: max_percent_full() const
    {
	string ret = "";

	map<U_I, mem_sized *>::const_iterator it = carte.begin();
	map<U_I, U_I>::const_iterator cit;
	U_I freq;

	ret += " ----------------------------------------------------\n";
	ret += " Statistical use of memory allocation per block size:\n";
	ret += " ----------------------------------------------------\n";
	while(it != carte.end())
	{
	    cit = count.find(it->first);
	    if(cit == count.end())
		freq = 0;
	    else
		freq = cit->second;
	    if(it->second == NULL)
		ret += tools_printf(" NULL reference associated to %d bytes blocks !?!?! (number of requests %d)\n",
				    it->first, freq);
	    else
		ret += tools_printf(" Usage for %d bytes blocks : %d %% (number of requests %d)\n",
				    it->first,
				    it->second->max_percent_full(),
				    freq);
	    ++it;
	}
	ret+= " ----------------------------------------------------\n";

	return ret;
    }
#endif


}  // end of namespace

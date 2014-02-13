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

#include "mem_sized.hpp"
#include "tools.hpp"

using namespace std;

namespace libdar
{

	//////////////////////////////////////////////
	// mem_sized class implementation
	//

    mem_sized::mem_sized(U_I block_size)
    {
	mem_cluster * tmp = NULL;

	if(block_size > 0)
	{
	    table_size_64 = average_table_size / (64 * block_size) + 1;
	    if(table_size_64 < 1)
		table_size_64 = 1;
	}
	else // for block_size == 0 we allocate only minimal sized mem_clusters
	    table_size_64 = 1;
	pending_release = NULL;
#ifdef LIBDAR_DEBUG_MEMORY
	sum_percent = 0;
	num_cluster = 0;
#endif
	tmp = new (nothrow) mem_cluster(block_size, table_size_64, this);
	if(tmp == NULL)
	    throw Ememory("mem_sized::mem_sized");
	try
	{
	    clusters.push_front(tmp);
	    next_free_in_table = clusters.begin();
	}
	catch(...)
	{
	    delete tmp;
	    throw;
	}
    }

    mem_sized::~mem_sized()
    {
	list<mem_cluster *>::iterator it = clusters.begin();

	while(it != clusters.end())
	{
	    if(*it != NULL)
		delete *it;
	    ++it;

	}
	clusters.clear();
	pending_release = NULL;
    }

    void *mem_sized::alloc(mem_allocator * & ptr)
    {
	while(next_free_in_table != clusters.end()
	      && (*next_free_in_table) != NULL
	      && ( (*next_free_in_table) == pending_release || (*next_free_in_table)->is_full() )
	    )
	    ++next_free_in_table;

	if(next_free_in_table == clusters.end())
	{
	    next_free_in_table = clusters.begin();

	    while(next_free_in_table != clusters.end()
		  && (*next_free_in_table) != NULL
		  && ( (*next_free_in_table) == pending_release || (*next_free_in_table)->is_full() )
		)
		++next_free_in_table;

	    if(next_free_in_table == clusters.end())
	    {
		if(pending_release == NULL)
		{
			// all clusters are full, we must allocate a new one

		    if(clusters.empty())
			throw SRC_BUG; // at least one mem_cluster should always be present
		    if((*clusters.begin()) == NULL)
			throw SRC_BUG; // all *mem_cluster should be valid objects
		    mem_cluster *tmp = new (nothrow) mem_cluster((*clusters.begin())->get_block_size(), table_size_64, this);
		    if(tmp == NULL)
			throw Ememory("mem_sized::alloc");
		    try
		    {
			clusters.push_front(tmp);
		    }
		    catch(...)
		    {
			delete tmp;
			throw;
		    }
		    next_free_in_table = clusters.begin();
		}
		else
		{
			// recycling the cluster that was pending for release

		    next_free_in_table = clusters.begin();
		    while(next_free_in_table != clusters.end() && *next_free_in_table != pending_release)
			++next_free_in_table;
			// next_free_in_table now points to pending_release mem_cluster

		    if(next_free_in_table == clusters.end())
			throw SRC_BUG;
		    pending_release = NULL; // we do not have anymore a cluster pending for release
		}
	    }
	}

	if(*next_free_in_table == NULL)
	    throw SRC_BUG;
	else
	    ptr = *next_free_in_table;

	return (*next_free_in_table)->alloc();
    }

    void mem_sized::push_to_release_list(mem_allocator *ref)
    {
	if(pending_release != NULL)
	{
	    list<mem_cluster *>::iterator it = clusters.begin();

#ifdef LIBDAR_DEBUG_MEMORY
	    sum_percent += pending_release->max_percent_full();
	    ++num_cluster;
#endif
	    while(it != clusters.end() && (*it) != pending_release)
		++it;

	    if(it == clusters.end())
		throw SRC_BUG; // cannot release previously recorded cluster
	    if(it == next_free_in_table)
		++next_free_in_table;
	    if(!pending_release->is_empty())
		throw SRC_BUG; // some blocks have been (re-)allocated from that cluster!
	    delete pending_release;
	    pending_release = NULL;
	    clusters.erase(it);
	    if(clusters.size() == 0)
		throw SRC_BUG; // should always have at least one cluster
	}

	pending_release = (mem_cluster *)ref;
    }

    bool mem_sized::is_empty() const
    {
	return clusters.size() == 1 && (*clusters.begin()) != NULL && (*clusters.begin())->is_empty();
    }

    string mem_sized::dump() const
    {
	string ret = "";
	list<mem_cluster *>::const_iterator it = clusters.begin();

	ret += tools_printf("   %d cluster(s) contain unreleased blocks of memory:\n", clusters.size());

	while(it != clusters.end())
	{
	    if(*it == NULL)
		ret += "  !?! NULL pointer in cluster list !?!\n";
	    else
	    {
		if(!(*it)->is_empty())
		    ret += (*it)->dump();
	    }
	    ++it;
	}

	return ret;
    }

#ifdef LIBDAR_DEBUG_MEMORY
    U_I mem_sized::max_percent_full() const
    {
	U_I tmp_sum = sum_percent;
	U_I tmp_num = num_cluster;

	if(pending_release != NULL)
	{
	    tmp_sum += pending_release->max_percent_full();
	    ++tmp_num;
	}

	return tmp_num > 0 ? tmp_sum / tmp_num : 0;
    }
#endif


}  // end of namespace

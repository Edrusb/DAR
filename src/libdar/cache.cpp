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
// to contact the author : dar.linux@free.fr
/*********************************************************************/
// $Id: cache.cpp,v 1.10.2.1 2005/03/13 20:07:49 edrusb Rel $
//
/*********************************************************************/

#include "../my_config.h"

#include <string.h>
#include "cache.hpp"

using namespace std;

namespace libdar
{

// This implementation of cache is a bit simple
// we do not need a read read-write caching scheme but rather a read or write caching scheme
// to make it possible to use for reading and writing (for coherence and avoid change in the use) the
// caching scheme works either in read caching or write caching alternatively depending on the read() or write()
// use.
//
// if a real read-write caching is required, this implementation can be completely reviewed without any
// change in its usage.  Currently we just need to gather several read or write on a single huge one.
//


    cache::cache(user_interaction & dialog,
		 generic_file & hidden,
		 U_I initial_size, U_I unused_read_ratio, U_I observation_read_number, U_I max_size_hit_read_ratio,
		 U_I unused_write_ratio, U_I observation_write_number, U_I max_size_hit_write_ratio) : generic_file(dialog, hidden.get_mode())
    {
	    // sanity checks
	if(&hidden == NULL)
	    throw Erange("cache::cache", gettext("NULL given as \"hidden\" argument while initializing cache"));
	if(initial_size < 1)
	    throw Erange("cache::cache", gettext("wrong value given as initial_size argument while initializing cache"));
	if(observation_read_number < 2)
	    throw Erange("cache::cache", gettext("too low value (< 10) given as observation_read_number argument while initializing cache"));
	if(observation_write_number < 2)
	    throw Erange("cache::cache", gettext("too low value (< 10) given as observation_write_number argument while initializing cache"));
	if(unused_read_ratio >= 50)
	    throw Erange("cache::cache", gettext("too high value (> 50) given as unused_read_ratio argument, while initializing cache"));
	if(unused_write_ratio >= max_size_hit_write_ratio)
	    throw Erange("cache::cache", gettext("unused_write_ratio must be less than max_size_hit_write_ratio, while initializing cache"));

	ref = & hidden;

	buffer_cache.buffer = new char[initial_size];
	if(buffer_cache.buffer == NULL)
	    throw Ememory("cache::cache");
	buffer_cache.size = initial_size;
	buffer_cache.next = 0;
	buffer_cache.last = 0;
	read_mode = false;

	read_obs = observation_read_number;
	read_unused_rate = unused_read_ratio;
	read_overused_rate = max_size_hit_read_ratio;

	write_obs = observation_write_number;
	write_unused_rate = unused_write_ratio;
	write_overused_rate = max_size_hit_write_ratio;

	stat_read_unused = 0;
	stat_read_overused = 0;
	stat_read_counter = 0;

	stat_write_overused = 0;
	stat_write_counter = 0;
    }

    S_I cache::inherited_read(char *a, size_t size)
    {
	U_I ret = 0;
	bool eof = false;

	if(!read_mode)  // we need to swap to read mode
	{
	    flush_write();
	    read_mode = true;
	}


	do
	{
	    if(buffer_cache.next >= buffer_cache.last) // no more data in cache
	    {
		fulfill_read();
		if(buffer_cache.next >= buffer_cache.last) // could not read anymore data
		    eof = true;
	    }

	    if(!eof)
	    {
		U_I needed = size - ret;
		U_I avail = buffer_cache.last - buffer_cache.next;
		U_I min = avail > needed ? needed : avail;

		(void)memcpy(a+ret, buffer_cache.buffer + buffer_cache.next, min);
		ret += min;
		buffer_cache.next += min;
	    }
	}
	while(ret < size && !eof);

	return ret;
    }


    S_I cache::inherited_write(const char *a, size_t size)
    {
	U_I wrote = 0;
	U_I avail, min;

	if(read_mode)
	{
	    clear_read();
	    read_mode = false;
	}

	while(wrote < size)
	{
	    avail = buffer_cache.size - buffer_cache.next;
	    if(avail == 0)
	    {
		flush_write();
		avail = buffer_cache.size - buffer_cache.next;
		if(avail == 0)
		    throw SRC_BUG;
	    }
	    min = avail > (size-wrote) ? (size-wrote) : avail;

	    (void)memcpy(buffer_cache.buffer + buffer_cache.next, a + wrote, min);
	    wrote += min;
	    buffer_cache.next += min;
	}

	return wrote;
    }


    void cache::flush_write()
    {
	if(get_mode() == gf_read_only || read_mode)
	    return; // nothing to flush

	    // computing stats

	stat_write_counter++;
	if(buffer_cache.next == buffer_cache.size) // cache is full
	    stat_write_overused++;

	    // flushing the cache

	if(buffer_cache.next > 0) // we have something to flush
	    ref->write(buffer_cache.buffer, buffer_cache.next);
	buffer_cache.next = 0;

	    // write cache is now empty
	    // eventually checking cache behavior

	if(stat_write_counter >= write_obs) // must compute stats and adapt the cache size
	{
	    if(stat_write_overused*100 > write_overused_rate*stat_write_counter) // need to increase the cache
	    {
		U_I tmp = buffer_cache.size * 2;
		if(buffer_cache.size < tmp)
		{
		    delete [] buffer_cache.buffer;
		    buffer_cache.buffer = NULL;
		    buffer_cache.size = tmp;
		    buffer_cache.buffer = new char[buffer_cache.size];
		    if(buffer_cache.buffer == NULL)
			throw Ememory("cache::flush_write");
		}
		    // else nothing is done, as we cannot get larger buffer
	    }
	    else
		if((stat_write_counter - stat_write_overused)*100 < write_unused_rate*stat_write_counter) // need to decrease the cache
		{
		    U_I tmp = buffer_cache.size / 2;
		    if(tmp < buffer_cache.size && tmp > 0)
		    {
			delete [] buffer_cache.buffer;
			buffer_cache.buffer = NULL;
			buffer_cache.size = tmp;
			buffer_cache.buffer = new char[buffer_cache.size];
			if(buffer_cache.buffer == NULL)
			    throw Ememory("cache::flush_write");
		    }
			// else nothing is done, as we cannot have a smaller buffer
		}
		// else we keep the same cache size
	    stat_write_overused = 0;
	    stat_write_counter = 0;
	}
	    // else this is not time for cache resizing

	    // reseting counters for statistics
    }

    static void dummy_call(char *x)
    {
        static char id[]="$Id: cache.cpp,v 1.10.2.1 2005/03/13 20:07:49 edrusb Rel $";
        dummy_call(id);
    }

    void cache::fulfill_read()
    {
	if(get_mode() == gf_write_only || !read_mode)
	    return; // nothing to fill

	    // computing stats

	stat_read_counter++;
	if(100 * buffer_cache.next < read_unused_rate * buffer_cache.last)
	    stat_read_unused++;
	if(buffer_cache.next == buffer_cache.last && buffer_cache.next != 0)
	    stat_read_overused++;

	    // all data in cache is lost, cache becomes empty at this point
	    // eventually checking cache behavior

	if(stat_read_counter >= read_obs)
	{
	    if(stat_read_overused * 100 > read_overused_rate * stat_read_counter) // need to increase cache size
	    {
		U_I tmp = buffer_cache.size * 2;
		if(buffer_cache.size < tmp)
		{
		    delete [] buffer_cache.buffer;
		    buffer_cache.buffer = NULL;
		    buffer_cache.size = tmp;
		    buffer_cache.buffer = new char[buffer_cache.size];
		    if(buffer_cache.buffer == NULL)
			throw Ememory("cache::fulfill_read");
		}
		    // else nothing is done as we cannot get a larger buffer
	    }
	    else
		if(stat_read_unused * 100 < read_unused_rate * stat_read_counter) // need to decrease cache size
		{
		    U_I tmp = buffer_cache.size / 2;
		    if(tmp < buffer_cache.size && tmp > 0)
		    {
			delete [] buffer_cache.buffer;
			buffer_cache.buffer = NULL;
			buffer_cache.size = tmp;
			buffer_cache.buffer = new char[buffer_cache.size];
			if(buffer_cache.buffer == NULL)
			    throw Ememory("cache::fulfill_read");
		    }
			// else nothing is done, as we cannot get asmaller buffer
		}
		// else the cache size does not need to be changed
	    stat_read_counter = 0;
	    stat_read_unused = 0;
	    stat_read_overused = 0;
	}
	    // else this is not time for cache resizing

	    // now fulfilling the cache

	buffer_cache.next = 0;
	buffer_cache.last = ref->read(buffer_cache.buffer, buffer_cache.size);
    }

} // end of namespace

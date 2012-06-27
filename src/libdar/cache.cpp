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

extern "C"
{
#if HAVE_LIMITS_H
#include <limits.h>
#endif

#if HAVE_STRING_H
#include <string.h>
#endif

#if HAVE_STRINGS_H
#include <strings.h>
#endif

#if STDC_HEADERS
# include <string.h>
#else
# if !HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (), *strrchr ();
# if !HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

}

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


    cache::cache(generic_file & hidden,
		 bool shift_mode,
		 U_I initial_size,
		 U_I unused_read_ratio,
		 U_I observation_read_number,
		 U_I max_size_hit_read_ratio,
		 U_I unused_write_ratio,
		 U_I observation_write_number,
		 U_I max_size_hit_write_ratio) : generic_file(hidden.get_mode())
    {
	    // sanity checks
	if(&hidden == NULL)
	    throw Erange("cache::cache", "NULL given as \"hidden\" argument while initializing cache"); // not translated message, this is expected
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
	current_position = ref->get_position();
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

	shifted_mode = shift_mode;
	failed_increase = false;
    }

    cache::~cache()
    {
	try
	{
	    flush_write();
	}
	catch(...)
	{
		// ignore all exceptions
	}
    }

    bool cache::skip(const infinint & pos)
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(!read_mode)
	{
	    flush_write();
	    if(pos != current_position || pos != ref->get_position())
		if(ref->skip(pos))
		{
		    current_position = pos;
		    return true;
		}
		else
		{
		    current_position = ref->get_position();
		    return false;
		}
	    else
		return true;
	}
	else // read mode
	    if(current_position <= pos + buffer_cache.next && pos <= current_position + buffer_cache.last - buffer_cache.next) // requested position already in the cache
	    {
		if(pos < current_position)
		{

			// skipping backward in cache

		    infinint delta = current_position - pos;
		    U_I delta_ui = 0;

		    current_position -= delta;
		    delta.unstack(delta_ui);
		    if(delta != 0)
			throw SRC_BUG;

		    if(delta_ui > buffer_cache.next)
			throw SRC_BUG;
		    buffer_cache.next -= delta_ui;
		}
		else
		{

			// skipping forward in cache


		    infinint delta = pos - current_position;
		    U_I delta_ui = 0;

		    current_position += delta;
		    delta.unstack(delta_ui);
		    if(delta != 0)
			throw SRC_BUG;

		    if(delta_ui + buffer_cache.next > buffer_cache.last)
			throw SRC_BUG;

		    buffer_cache.next += delta_ui;
		}
		return true;
	    }
	    else // position out of the cache
	    {
		bool ret;

		clear_read();
		ret = ref->skip(pos);
		current_position = pos;
		return ret;
	    }
    }

    bool cache::skip_to_eof()
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(!read_mode)
	{
	    bool ret;

	    flush_write();
	    ret = ref->skip_to_eof();
	    current_position = ref->get_position();
	    return ret;
	}
	else
	{
	    bool ret = ref->skip_to_eof();
	    if(ret)
		return skip(ref->get_position());
	    else
		return ret;
	}
    }

    bool cache::skip_relative(S_I x)
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(!read_mode)
	{
	    bool ret;

	    flush_write();
	    ret = ref->skip_relative(x);
	    current_position = ref->get_position();
	    return ret;
	}
	else // read mode
	{
	    if(x >= 0)
		return skip(current_position + x);
	    else
	    {
		if(current_position >= -x) // -x is positive (because x is negative)
		    return skip(current_position - infinint(-x));
		else
		{
		    current_position = 0;
		    (void)skip(0);
		    return false;
		}
	    }
	}
    }

    U_I cache::inherited_read(char *a, U_I size)
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
	current_position += ret;

	return ret;
    }


    void cache::inherited_write(const char *a, U_I size)
    {
	U_I wrote = 0;
	U_I avail, min;

	if(read_mode)
	{
	    clear_read();
	    read_mode = false;
	}

	try
	{
	    while(wrote < size)
	    {
		avail = buffer_cache.size - buffer_cache.next;
		if(avail == 0) // we need to flush the cache
		{
		    flush_write();
		    avail = buffer_cache.size - buffer_cache.next;
		    if(avail == 0)
			throw SRC_BUG;
		    if(avail < size - wrote)
		    {
			ref->write(a + wrote, size - wrote);
			wrote = size;
			continue; // ending the while loop now, as there's no data left to copy
		    }
		}
		min = avail > (size-wrote) ? (size-wrote) : avail;

		(void)memcpy(buffer_cache.buffer + buffer_cache.next, a + wrote, min);
		wrote += min;
		buffer_cache.next += min;
	    }
	    current_position += wrote;
	}
	catch(...)
	{
	    current_position = ref->get_position() + buffer_cache.next;
	    throw;
	}
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

	    // flushing / shifting the cache contents

	if(shifted_mode)
	    buffer_cache.shift_by_half();
	else
	    buffer_cache.clear();

	    ///////
	    // some data may remain in the cache, we need to preserve them !!!
	    // this occurres when a shift by half of the buffer has been done just before
	    ///////

	if(stat_read_counter >= read_obs)
	{
	    if(stat_read_overused * 100 > read_overused_rate * stat_read_counter) // need to increase cache size
	    {
		if(!failed_increase)
		{
		    U_I tmp = buffer_cache.size * 2;
#ifdef SSIZE_MAX
		    if(tmp <= SSIZE_MAX && buffer_cache.size < tmp)
#else
		    if(buffer_cache.size < tmp)
#endif
		    {
			try
			{
			    buffer_cache.resize(tmp);
			}
			catch(Ememory & e)
			{
			    failed_increase = true;
			}
			catch(bad_alloc & e)
			{
			    failed_increase = true;
			}
		    }
		}
	    }
	    else
		if(stat_read_unused * 100 < read_unused_rate * stat_read_counter) // need to decrease cache size
		{
		    U_I tmp = 2 * buffer_cache.size / 3;
		    if(tmp < buffer_cache.size && tmp > 0 && tmp > buffer_cache.last) // so we have enough room in the new buffer for in place data
			buffer_cache.resize(tmp);
			// else nothing is done, as we cannot get a smaller buffer
		    failed_increase = false;
		}
		// else the cache size does not need to be changed
	    stat_read_counter = 0;
	    stat_read_unused = 0;
	    stat_read_overused = 0;
	}
	    // else this is not time for cache resizing

	    // now fulfilling the cache


	if(buffer_cache.next != buffer_cache.last)
	    throw SRC_BUG;
	    // should not call fullfill_read unless all data has been read
	else
	{
	    U_I lu = ref->read(buffer_cache.buffer + buffer_cache.last, buffer_cache.size - buffer_cache.last);
	    buffer_cache.last += lu;
	}
    }

///////////////////////////// cache::buf methods /////////////////////////////////

    void cache::buf::resize(U_I newsize)
    {
	char *tmp = NULL;

	if(newsize < last)
	    throw SRC_BUG;

	tmp = new char[newsize];
	if(tmp == NULL)
	    throw Ememory("cache::buf::resize");

	try
	{
	    (void)memcpy(tmp, buffer, last);
	    if(buffer == NULL)
		throw SRC_BUG;
	    delete[] buffer;
	    buffer = tmp;
	    size = newsize;
	}
	catch(...)
	{
	    if(tmp != NULL)
	    {
		delete tmp;
		tmp = NULL;
	    }
	    throw;
	}
    }

    void cache::buf::shift_by_half()
    {
	U_I half = last / 2;
	U_I reste = last % 2;

	if(last > 1)
	{
	    (void)memmove(buffer, buffer + half, half+reste);
	    next -= half;
	    last -= half;
	}
    }


} // end of namespace




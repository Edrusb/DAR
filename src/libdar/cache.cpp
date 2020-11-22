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

    cache::cache(generic_file & hidden,
		 bool shift_mode,
		 U_I x_size) : generic_file(hidden.get_mode())
				   // except if hidden is read-only we provide read-write facility in
				   // the cache, for what is out of the cache we check
				   // the underlying object mode
    {
	    // sanity checks
	if(x_size < 10)
	    throw Erange("cache::cache", gettext("wrong value given as initial_size argument while initializing cache"));

	ref = & hidden;
	buffer = nullptr;
	alloc_buffer(x_size);
	next = 0;
	last = 0;
	first_to_write = size;
	buffer_offset = ref->get_position();
	shifted_mode = shift_mode;
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
	release_buffer();
    }

    bool cache::skippable(skippability direction, const infinint & amount)
    {
	infinint in_cache = available_in_cache(direction);

	    // either available data is enough to assure skippability or we
	    // calculate the direction and amount to ask to the lower layer (ref)

	if(in_cache >= amount)
	{
		// skippability is contained in the cached data

	    switch(direction)
	    {
	    case skip_forward:
		return true;

	    case skip_backward:
		    // sanity check
		if(infinint(next) < amount)
		    throw SRC_BUG;

		if(first_to_write != size)
		{
			// some data is pending for writing

			// the cached data before first_to_write has already
			// been written to the cached layer, if we have to
			// skip before this position, we have to check the
			// cached object will allow skipping backward to
			// overwrite the already written data

		    infinint new_ftw = infinint(next) - amount;
		    if(infinint(first_to_write) <= new_ftw)
			    // we don't lead the cached object to skip backward
			    // in order to overwrite data
			return true;
		    else // we must check whether the cached object will allow skipping backward
		    {
			infinint backw = infinint(first_to_write) - new_ftw;
			return ref->skippable(skip_backward, backw);
		    }
		}
		else // no write pending data
		    return true;

	    default:
		throw SRC_BUG;
	    }

	    return true;
	}
	else
	{
	    switch(direction)
	    {
	    case skip_forward:
		if(ref->get_position() <= buffer_offset)
		    return ref->skippable(direction, buffer_offset - ref->get_position() + next + amount);
		else
		{
		    infinint backw = ref->get_position() - buffer_offset;
		    infinint forw = amount + next;
		    if(backw >= forw)
			return ref->skippable(skip_backward, backw - forw);
		    else
			return ref->skippable(skip_forward, forw - backw);
		}
	    case skip_backward:
		    // if we don't flush write pending data, the cached object can evaluate backward
		    // skippable possible while it could not be possible once the data pending for
		    // writing will be flushed (sar object for example)
		    // But flushing data now, does not remove it all from the cache and some may remain
		    // which could later be modified from the cache only. Then once time will come to
		    // flush this data, this would mean for the cached object to skip backward to modify
		    // the already flushed data. Skipping backward may be impossible at all (tuyau) so the
		    // data flushing would fail and the global write operation on the cache object
		    // would also
		    // so we must flush write pending data
		    // but control the first_to_write field skippability is contained insde the cache
		    // for that when it decreases a check is done for backward skippability on the cached object
		if(need_flush_write())
		    flush_write(); // before skipping we would first have to write the pending data so we do it
		    // now for the underlay provides a coherent answer from skippable now with the effective skip at later time
		if(ref->get_position() >= buffer_offset)
		{
		    infinint backw = ref->get_position() - buffer_offset  + amount;
		    infinint forw = next;
		    if(backw >= forw)
			return ref->skippable(skip_backward, backw - forw);
		    else
			return ref->skippable(skip_forward, forw - backw);
		}
		else
		{
		    infinint backw = amount;
		    infinint forw = buffer_offset - ref->get_position()  + next;
		    if(backw >= forw)
			return ref->skippable(skip_backward, backw - forw);
		    else
			return ref->skippable(skip_forward, forw - backw);
		}
	    default:
		throw SRC_BUG;
	    }
	}
    }

    bool cache::skip(const infinint & pos)
    {
	if(is_terminated())
	    throw SRC_BUG;

	if(pos >= buffer_offset && pos <= buffer_offset + last)
	{
		// skipping inside the buffer is possible

	    infinint tmp_next = pos - buffer_offset;
	    U_I new_next = 0;

	    tmp_next.unstack(new_next);
	    if(!tmp_next.is_zero())
		throw SRC_BUG;

		// assigning to next its new value to reflect the skip() operation

	    if(first_to_write > new_next && first_to_write != size)
	    {
		    // reducing first_to_write means the cached object
		    // will have to skip backward, this we must check
		if(ref->skippable(skip_backward, first_to_write - new_next))
		    first_to_write = new_next;
		else
		    return false; // cannot skip
	    }
	    next = new_next;

	    return true;
	}
	else // skipping would lead the current position to be outside the buffer
	{
	    bool ret;

	    if(need_flush_write())
		flush_write();
	    next = last = 0;
	    ret = ref->skip(pos);
	    buffer_offset = ref->get_position();

	    return ret;
	}
    }

    bool cache::skip_to_eof()
    {
	bool ret;
	infinint max;

	if(is_terminated())
	    throw SRC_BUG;

	if(need_flush_write())
	    flush_write();

	if(eof_offset.is_zero())
	{
	    ret = ref->skip_to_eof();
	    eof_offset = ref->get_position();
	}
	else
	    ret = skip(eof_offset);

	if(buffer_offset + last < eof_offset)
	{
	    clear_buffer();
	    buffer_offset = eof_offset;
	}
	else
	{
	    next = last;
	    if(buffer_offset + last > eof_offset)
		throw SRC_BUG;
	}

	return ret;
    }

    bool cache::skip_relative(S_I x)
    {
	skippability dir = x >= 0 ? skip_forward : skip_backward;
	U_I in_cache = available_in_cache(dir);
	U_I abs_x = x >= 0 ? x : -x;

	if(is_terminated())
	    throw SRC_BUG;

	if(abs_x <= in_cache) // skipping within cache
	{
	    next += x; // note that x is a *signed* integer

		// sanity checks

	    if(next > last)
		throw SRC_BUG;
	    return true;
	}
	else // must replace data in cache to skip
	{
	    if(need_flush_write())
		flush_write();

	    switch(dir)
	    {
	    case skip_forward:
		return skip(buffer_offset + abs_x);
	    case skip_backward:
		if(buffer_offset < abs_x)
		    return false;
		else
		    return skip(buffer_offset - abs_x);
	    default:
		throw SRC_BUG;
	    }
	}
    }

    void cache::inherited_read_ahead(const infinint & amount)
    {
	infinint tmp = available_in_cache(generic_file::skip_forward);

	if(amount > tmp)
	    ref->read_ahead(amount - tmp);
	    // else requested data is already in cache
    }


    U_I cache::inherited_read(char *a, U_I x_size)
    {
	U_I ret = 0;
	bool eof = false;
	infinint fallback_offset = get_position();

	do
	{
	    if(next >= last) // no more data to read from cache
	    {
		try
		{
		    if(need_flush_write())
			flush_write();
		}
		catch(...)
		{
		    if(next < ret)
			throw SRC_BUG;
			// flushing failed thus cache
			// should contains at least ret bytes
		    next -= ret;
			// this way we don't loose what has to be
			// written for long, while dropping the
			// bytes put in the cache by the current
			// inherited_read routine. The caller may
			// considering that no byte has been read
			// due to the exception next call will
			// provide the expected data
		    throw;
		}

		try
		{
		    if(x_size - ret < size)
		    {
			if(eof_offset.is_zero()                    // we know the offset for eof
			   || buffer_offset + last < eof_offset)   // we have not all data up to eof
			    fulfill_read(); // may fail if underlying is write_only (exception thrown)
			if(next >= last) // could not read anymore data
			    eof = true;
		    }
		    else // reading the remaining directly from lower layer
		    {
			ret += ref->read(a + ret, x_size - ret); // may fail if underlying is write_only
			if(ret < x_size)
			    eof = true;
			clear_buffer();   // force clearing whatever is shifted_mode
			buffer_offset = ref->get_position();
		    }
		}
		catch(...)
		{
		    skip(fallback_offset);
		    throw;
		}
	    }

	    if(!eof && ret < x_size)
	    {
		U_I needed = x_size - ret;
		U_I avail = last - next;
		U_I min = avail > needed ? needed : avail;

		if(min > 0)
		{
		    (void)memcpy(a+ret, buffer + next, min);
		    ret += min;
		    next += min;
		}
		else
		    throw SRC_BUG;
	    }
	}
	while(ret < x_size && !eof);

	return ret;
    }


    void cache::inherited_write(const char *a, U_I x_size)
    {
	U_I wrote = 0;
	U_I avail, remaining;

	while(wrote < x_size)
	{
	    avail = size - next;
	    if(avail == 0) // we need to flush the cache
	    {
		try
		{
		    if(need_flush_write())
			flush_write();    // may fail if underlying is read_only
		    avail = size - next;
		}
		catch(...)
		{
			// ignoring the bytes written so far from
			// the given argument to inherited_write in
			// order to stay coherent with the view of the caller
		    if(next < wrote)
			throw SRC_BUG;
		    next -= wrote;
		    throw;
		}
	    }

	    remaining = x_size - wrote;
	    if(avail < remaining && !need_flush_write())
	    {
		    // less data in cache than to be wrote and no write pending data  in cache
		    // we write directly to the lower layer

		buffer_offset += next;
		next = last = 0;
		try
		{
		    ref->skip(buffer_offset);
		    ref->write(a + wrote, remaining); // may fail if underlying is read_only or user interruption
		}
		catch(...)
		{
		    infinint wrote_i = wrote;
			// ignoring the bytes written so far from
			// the given argument to inherited_write in
			// order to stay coherent with the view of the caller
		    if(buffer_offset < wrote_i)
			throw SRC_BUG;
		    buffer_offset -= wrote_i;
		    ref->skip(buffer_offset);
		    throw;
		}
		wrote = x_size;
		buffer_offset += remaining;
	    }
	    else // filling cache with data
	    {
		U_I min = remaining < avail ? remaining : avail;
		if(first_to_write >= last)
		    first_to_write = next;
		(void)memcpy(buffer + next, a + wrote, min);
		wrote += min;
		next += min;
		if(last < next)
		    last = next;
	    }
	}
	if(wrote > 0 && !eof_offset.is_zero())
	    eof_offset = 0;
    }

    void cache::inherited_truncate(const infinint & pos)
    {
	if(pos >= buffer_offset + last)
	    ref->truncate(pos); // no impact on the cached data
	else if(pos < buffer_offset)
	{
	    next = 0;   // emptying the cache
	    last = 0;   // emptying the cache
	    first_to_write = size; // dropping data pending for writing
	    ref->truncate(pos);
	    buffer_offset = ref->get_position();
	    if(buffer_offset != pos)
		throw SRC_BUG;
	}
	else // truncate in the middle of the cache
	{
		// we have: buffer_offset <= pos < buffer_offset + last
	    infinint max_offset = pos - buffer_offset;
	    U_I max_off = 0;

	    max_offset.unstack(max_off);
	    if(!max_offset.is_zero())
		throw SRC_BUG;

	    if(last > max_off)
		last = max_off;
	    if(next > max_off)
		next = max_off;
	    if(first_to_write >= last) // there is nothing more pending for writing
		first_to_write = size;

	    ref->truncate(pos);
	}
    }

    void cache::alloc_buffer(size_t x_size)
    {
	if(buffer != nullptr)
	    throw SRC_BUG;

	buffer = new (nothrow) char[x_size];
	if(buffer == nullptr)
	    throw Ememory("cache::alloc_buffer");
	size = x_size;
	half = size / 2;
    }

    void cache::release_buffer()
    {
	if(buffer == nullptr)
	    throw SRC_BUG;

	delete [] buffer;
	buffer = nullptr;
	size = 0;
	half = 0;
    }

    void cache::shift_by_half()
    {
	U_I shift;

	if(last <= half)
	    return; // not enough data to keep the cache filled by half
	else
	    shift = last - half;

	if(next < shift)
	    shift = next; // current position cannot be out of the buffer so we don't shift more than "next"

	if(first_to_write < shift)
	    throw SRC_BUG;

	(void)memmove(buffer, buffer + shift, last - shift);
	if(first_to_write < size)
	    first_to_write -= shift;
	next -= shift;
	last -= shift;
	buffer_offset += shift;
    }

    void cache::clear_buffer()
    {
	if(need_flush_write())
	    throw SRC_BUG;
	buffer_offset += next;
	next = last = 0;
    }

    void cache::flush_write()
    {
	if(get_mode() == gf_read_only)
	    return; // nothing to flush

	    // flushing the cache

	if(need_flush_write()) // we have something to flush
	{
	    if(!ref->skip(buffer_offset + first_to_write))
		throw SRC_BUG; // cannot flush write data !!!
	    ref->write(buffer + first_to_write, last - first_to_write);
	}
	first_to_write = size;
	next = last; // we have wrote up to last so we must update next for clear_buffer() to work as expected

	if(shifted_mode)
	    shift_by_half();
	else
	    clear_buffer();
    }

    void cache::fulfill_read()
    {
	U_I lu;
	bool skipping = (last == 0);

	if(get_mode() == gf_write_only)
	    return; // nothing to fill

	    // flushing / shifting the cache contents to make room to receive new data

	if(shifted_mode)
	    shift_by_half();
	else
	    clear_buffer();

	    ///////
	    // some data may remain in the cache, we need to preserve them !!!
	    // this occurres when a shift by half of the buffer has been done just before
	    ///////

	if(!eof_offset.is_zero()                       // eof position is known
	   && buffer_offset + last + size > eof_offset // we would read up to the eof
	   && next == last                             // cache is currently empty
	   && skipping)                                // we got there due to a call to skip()
	{
	    infinint tmp_next;

		// we will read the last block size of the fill
		// and put it into the cache. This way backward
		// reading does not bring any performance penalty
		// when it occurs at end of file

	    if(eof_offset > size)
	    {
		    // setting the value for "next"
		tmp_next = (buffer_offset + size) - eof_offset;
		    // parenthesis are required to avoid substracting before addition and obtaining an
		    // negative infinint as temporary object
		next = 0;
		tmp_next.unstack(next);
		if(!tmp_next.is_zero())
		    throw SRC_BUG;

		    // setting the value for "buffer_offset"
		buffer_offset = eof_offset - size;

		if(!ref->skip(buffer_offset))
		    throw SRC_BUG;
	    }
	    else // file is shorter than the cache size!
	    {
		    // setting the value for "next"
		tmp_next = buffer_offset;
		next = 0;
		tmp_next.unstack(next);
		if(!tmp_next.is_zero())
		    throw SRC_BUG;

		    // setting the value for "buffer_offset"
		buffer_offset = 0;

		if(!ref->skip(0))
		    throw SRC_BUG;
	    }
	}
	else
	{
		// "next" and "buffer_offset" do not change

	    if(!ref->skip(buffer_offset + last))
		throw SRC_BUG;
	}
	lu = ref->read(buffer + last, size - last); // may fail if underlying is write_only or user aborted
	last += lu;
    }

    U_I cache::available_in_cache(skippability direction) const
    {
	U_I ret;

	switch(direction)
	{
	case skip_forward:
	    ret = last - next;
	    break;
	case skip_backward:
	    ret = next;
	    break;
	default:
	    throw SRC_BUG;
	}

	return ret;
    }

} // end of namespace

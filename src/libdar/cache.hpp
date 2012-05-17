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
// $Id: cache.hpp,v 1.9.2.1 2005/03/13 20:07:50 edrusb Rel $
//
/*********************************************************************/

    /// \file cache.hpp
    /// \brief contains the cache class
    /// \ingroup Private

#ifndef CACHE_HPP
#define CACHE_HPP

#include "../my_config.h"
#include "infinint.hpp"
#include "generic_file.hpp"

namespace libdar
{
	/// the cache class implements a very basic read/write caching mechanisme

	/// it is intended to reduce context switches when no compression is used
	/// and when reading or writing catalogue through a pipe. The catalogue
	/// read and write is done by calling dump/constructor methods of the many
	/// objects that a catalogue can contain. This makes a lot of small read
    	/// or write, which make very poor performances when used over the network
	/// through a pipe to ssh. When compression is used, the problem disapears
	/// as the compression engine gather theses many small read or write into
	/// much bigger ones. This in only when there is no compression that
	/// that this class is useful (and used).
	/// \ingroup Private
    class cache : public generic_file
    {
    public:
	cache(user_interaction & dialog,
	      generic_file & hidden,
	      U_I initial_size = 10240,
              U_I unused_read_ratio = 10, U_I observation_read_number = 100, U_I max_size_hit_read_ratio = 50,
	      U_I unused_write_ratio = 10, U_I observation_write_number = 100, U_I max_size_hit_write_ratio = 50);
	    // *** hidden: is the file to cache, it is never deleted by the cache object,
	    // *** intial_size: is the initial size of the cache for reading (read this amount of data at a time)
	    // unused_read_ratio: is the ratio of cached data effectively asked for reading below which the cache size is divided by two
            //   in other words, if during observation_read_number fullfilment of the cache, less than unused_read_ratio percent of data has been
            //   effectively asked for reading, then the cache size is divided by two
            // *** max_size_hit_read_ratio: is the ratio above which the cache size is doubled. In other words, if during observation_read_number
	    //   times of cache fullfilment, more than max_size_hit_read_ratio percent time all the cached data has been asked for reading
	    //   the cache size is doubled.
	    // to have fixed size read caching, you can set unused_read_ratio to zero and max_size_hit_read_ratio to 101 or above.
	    // *** unused_write_ratio: cache is flushed before being fullfilled if a seek operation has been asked. If after observation_write_number
            //   of cache flushing, less than unused_write_ratio percent of the time the cache was full, its size is divided by 2
            // *** max_size_hit_write_ratio: if after observation_write_number of cache flushing, max_size_hit_write_ratio percent of the time
	    // the cache was fulled, its size get doubled.
            // to have a fixed size write caching , you can set unused_write_ratio to zero and max_size_hit_write_ratio to 101 or above
	~cache() { flush_write(); };


	    // inherited from generic_file
	bool skip(const infinint & pos) { flush_write(); clear_read(); return ref->skip(pos); };
	bool skip_to_eof() { flush_write(); clear_read(); return ref->skip_to_eof(); };
	bool skip_relative(S_I x) { flush_write(); clear_read(); return ref->skip_relative(x); };
	infinint get_position() { return ref->get_position(); };

    protected:
	    // inherited from generic_file
	S_I inherited_read(char *a, size_t size);
	S_I inherited_write(const char *a, size_t size);

    private:
	struct buf
	{
	    char *buffer;
	    U_I size; // allocated size
	    U_I next; // next to read or next place to write to
	    U_I last; // first to not read in the cache

	    buf() { buffer = NULL; size = next = last = 0; };
	    ~buf() { if(buffer != NULL) delete [] buffer; };
	};

	generic_file *ref;

	struct buf buffer_cache;
	bool read_mode; // true if in read mode, false if in write mode

	U_I read_obs;
	U_I read_unused_rate;
	U_I read_overused_rate;

	U_I write_obs;
	U_I write_unused_rate;
	U_I write_overused_rate;

	U_I stat_read_unused;
	U_I stat_read_overused;
	U_I stat_read_counter;

	U_I stat_write_overused;
	U_I stat_write_counter;

	void flush_write();
	void fulfill_read();
	void clear_read() { if(read_mode) { buffer_cache.next = buffer_cache.last = 0; } };
    };

} // end of namespace

#endif


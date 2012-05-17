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
// $Id: cache.hpp,v 1.26 2011/04/17 13:12:29 edrusb Rel $
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

	/// \ingroup Private
	/// @}

	/// the cache class implements a very basic read/write caching mechanism

	/// it is intended to reduce context switches when no compression is used
	/// and when reading or writing catalogue through a pipe. The catalogue
	/// read and write is done by calling dump/constructor methods of the many
	/// objects that a catalogue can contain. This makes a lot of small read
    	/// or write, which make very poor performances when used over the network
	/// through a pipe to ssh. When compression is used, the problem disapears
	/// as the compression engine gather these many small read or write into
	/// much bigger ones. This in only when there is no compression that
	/// that this class is useful (and used).
    class cache : public generic_file
    {
    public:
	cache(generic_file & hidden, 	          //< is the file to cache, it is never deleted by the cache object,
	      bool shift_mode,                    //< if true, when all cached data has been read, half of the data is flushed from the cache, the other half is shifted and new data take place to fill the cache. This is necessary for sequential reading, but has some CPU overhead.
	      U_I initial_size = 10240,           //< is the initial size of the cache for reading (read this amount of data at a time)
              U_I unused_read_ratio = 10,         //< is the ratio of cached data effectively asked for reading below which the cache size is divided by two in other words, if during observation_read_number fullfilment of the cache, less than unused_read_ratio percent of data has been effectively asked for reading, then the cache size is divided by two
	      U_I observation_read_number = 100,  //< period of cache size consideration
	      U_I max_size_hit_read_ratio = 50,   //< is the ratio above which the cache size is doubled. In other words, if during observation_read_number times of cache fullfilment, more than max_size_hit_read_ratio percent time all the cached data has been asked for reading the cache size is doubled. To have fixed size read caching, you can set unused_read_ratio to zero and max_size_hit_read_ratio to 101 or above.
	      U_I unused_write_ratio = 10,        //< same as unused_read_ratio but for writing operations
	      U_I observation_write_number = 100, //< same as observation_read_number but for writing operations
	      U_I max_size_hit_write_ratio = 50); //< same as max_size_hit_read_ratio but for writing operations

	~cache();


	    // inherited from generic_file
	bool skip(const infinint & pos);
	bool skip_to_eof();
	bool skip_relative(S_I x);
	infinint get_position() { return current_position; };

    protected:
	    // inherited from generic_file
	U_I inherited_read(char *a, U_I size);
	void inherited_write(const char *a, U_I size);
	void inherited_sync_write() { flush_write(); };
	void inherited_terminate() { flush_write(); };
    private:
	struct buf
	{
	    char *buffer;
	    U_I size; // allocated size
	    U_I next; // next to read or next place to write to
	    U_I last; // first to not read in the cache

	    buf() { buffer = NULL; size = next = last = 0; };
	    buf(const buf &ref) { throw SRC_BUG; };
	    ~buf() { if(buffer != NULL) delete [] buffer; };
	    void resize(U_I newsize);
	    void shift_by_half();
	    void clear() { next = last = 0; };
	};

	generic_file *ref;                //< underlying file, (not owned by "this', not to be delete by "this")

	struct buf buffer_cache;          //< where is stored cached data
	infinint current_position;        //< current offset in file to read from or write to
	bool read_mode;                   //< true if in read mode, false if in write mode
	bool shifted_mode;                //< whether to half flush and shift or totally flush data
	bool failed_increase;             //< whether we failed increasing the cache size

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
	void clear_read() { if(read_mode) buffer_cache.clear(); };
    };

	/// @}

} // end of namespace

#endif


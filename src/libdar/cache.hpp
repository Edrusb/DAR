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

	/// the cache class implements a fixed length read/write caching mechanism
	///
	/// it is intended to reduce context switches when no compression is used
	/// and when reading or writing catalogue through a pipe. The catalogue
	/// read and write is done by calling dump/constructor methods of the many
	/// objects that a catalogue contains. This makes a lot of small reads or
    	/// writes, which make very poor performances when used over the network
	/// through a pipe to ssh. When compression is used, the problem disapears
	/// as the compression engine gather these many small reads or writes into
	/// much bigger ones. This in only when there is no compression or encryption
	/// that this class is useful (and used).
	/// Another target of class cache is to provide limited skippability when
	/// data is read of written to pipe (which do not have any skippability)
    class cache : public generic_file
    {
    public:
	cache(generic_file & hidden, 	          //< is the file to cache, it is never deleted by the cache object,
	      bool shift_mode,                    //< if true, when all cached data has been read, half of the data is flushed from the cache, the other half is shifted and new data take place to fill the cache. This is necessary for sequential reading, but has some CPU overhead.
	      U_I size = 102400);                 //< is the (fixed) size of the cache
	cache(const cache & ref) : generic_file(ref.get_mode()) { throw SRC_BUG; };
	const cache & operator = (const cache & ref) { throw SRC_BUG; };
	~cache();
	void change_to_read_write() { if(get_mode() == gf_read_only) throw SRC_BUG; set_mode(gf_read_write); };

	    // inherited from generic_file

	bool skippable(skippability direction, const infinint & amount);
	bool skip(const infinint & pos);
	bool skip_to_eof();
	bool skip_relative(S_I x);
	infinint get_position() const { return buffer_offset + next; };

    protected:
	    // inherited from generic_file
	void inherited_read_ahead(const infinint & amount);
	U_I inherited_read(char *a, U_I size);
	void inherited_write(const char *a, U_I size);
	void inherited_sync_write() { flush_write(); };
	void inherited_flush_read() { flush_write(); clear_buffer(); };
	void inherited_terminate() { flush_write(); };

    private:
	generic_file *ref;                //< underlying file, (not owned by "this', not to be delete by "this")
	char *buffer;                     //< data in transit
	U_I size;                         //< allocated size
	U_I half;                         //< precalculated half = size / 2
	U_I next;                         //< next to read or next place to write to
	U_I last;                         //< first byte of invalid data in the cache. we have: next <= last < size
	U_I first_to_write;               //< position of the first byte that need to be written. if greater than last, no byte need writing
	infinint buffer_offset;           //< position of the first byte in buffer
	bool shifted_mode;                //< whether to half flush and shift or totally flush data
	infinint eof_offset;              //< size of the underlying file (read-only mode), set to zero if unknown

	bool need_flush_write() const { return first_to_write < last; };
	void alloc_buffer(size_t x_size); //< allocate x_size byte in buffer field and set size accordingly
	void release_buffer();            //< release memory set buffer to nullptr and size to zero
	void shift_by_half();
	void clear_buffer();
	void flush_write();
	void fulfill_read();
	U_I available_in_cache(skippability direction) const;
    };

	/// @}

} // end of namespace

#endif


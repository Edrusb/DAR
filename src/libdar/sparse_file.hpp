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

    /// \file sparse_file.hpp
    /// \brief class sparse_file definition, used to handle holes in files
    /// \ingroup Private
    ///
    /// this class is used to receive plain file's data to be written to the
    /// archive or to be read out from an archive. The class uses escape sequences
    /// to replace holes in files (long serie of zeros) by the number of zeros
    /// preceeded by a escape sequence mark.
    /// this class internally uses an escape object, with a modifed
    /// fixed escape sequence that optimizes the use of sparse_file objects with
    /// other escape objects.

#ifndef SPARSE_FILE_HPP
#define SPARSE_FILE_HPP

#include "../my_config.h"

extern "C"
{
#if HAVE_LIMITS_H
#include <limits.h>
#endif
}

#include "generic_file.hpp"
#include "escape.hpp"

#define SPARSE_FIXED_ZEROED_BLOCK 40960
#ifdef SSIZE_MAX
#if SSIZE_MAX < MAX_BUFFER_SIZE
#undef MAX_BUFFER_SIZE
#define SPARSE_FIXED_ZEROED_BLOCK SSIZE_MAX
#endif
#endif

namespace libdar
{

	/// \addtogroup Private
	/// @{

    class sparse_file : public escape
    {
    public:
	    /// constructor

	    /// \param[in] below object where to read or write data to or from
	    /// \param[in] hole_size the size below which to ignore holes (zeroed bytes)
	    /// this parameter is only used if "below" is in write-only mode
	sparse_file(generic_file *below, const infinint & hole_size = 15);
	sparse_file(const sparse_file & ref) = default;
	sparse_file(sparse_file && ref) noexcept = default;
	sparse_file & operator = (const sparse_file & ref) = default;
	sparse_file & operator = (sparse_file && ref) noexcept = default;
	~sparse_file() = default;

	    /// \note if set to true, inherited_write() call will not make any
	    /// lookup for holes, the written data will simply be escaped if it
	    /// could collide with a mark used to signal the start of a hole
	void write_as_escape(bool mode) { escape_write = mode; };

	    /// \note  if set to true, the data will be unescaped or eof will
	    /// be signaled to the first mark met, instead of interpreting the
	    /// mark and what follows as a hole data structure.
	void read_as_escape(bool mode) { escape_read = mode; };

	    /// \note if set to true, the copy_to() methods, write zeroed data
	    /// in place of skipping over a hole to restore it into the target
	    /// generic_file
	void copy_to_without_skip(bool mode) { copy_to_no_skip = mode; };

	bool has_seen_hole() const { return seen_hole; };
	bool has_escaped_data() const { return data_escaped; };

	    /// copies data of the current object using holes to the given
	    /// generic_file, overwrites the generic_file method

	    /// \note, this assumes the underlying generic_file (where the sparse_file object
	    /// reads its data before writing it to "ref") uses a special
	    /// datastructure (which relies on class escape). This datastructure
	    /// is created by sparse_file in write_only mode (thus when one is writing
	    /// data to a sparse_file object, the underlying generic_file get a mix of
	    /// normal data and hole (number of zeroed bytes to skip to reach next normal data)
	    /// leading to only restore the data not equal to zero (passed a certain
	    /// amount of contiguous zeroed bytes).
	virtual void copy_to(generic_file & ref) override { crc *tmp = nullptr; copy_to(ref, 0, tmp); if(tmp != nullptr) throw SRC_BUG; };

	    /// same as sparse_file::copy_to(generic_file) just above but here with crc,

	    /// \note this also overwrite the corresponding class generic_file method, as we need a specific implementation here
	virtual void copy_to(generic_file & ref, const infinint & crc_size, crc * & value) override;

	    // indirectly inherited from generic_file
	virtual bool skippable(skippability direction, const infinint & amount) override { return false; };
	virtual bool skip(const infinint & pos) override { if(pos != offset) throw Efeature("skip in sparse_file"); else return true; };
	virtual bool skip_to_eof() override { throw Efeature("skip in sparse_file"); };
	virtual bool skip_relative(S_I x) override { if(x != 0) throw Efeature("skip in sparse_file"); return true; };
	virtual infinint get_position() const override;

    protected:

	    // methods from the escape class we hide from the (public) class interface

	void add_mark_at_current_position(sequence_type t) { escape::add_mark_at_current_position(t); };
	bool skip_to_next_mark(sequence_type t, bool jump) { return escape::skip_to_next_mark(t, jump); };
	bool next_to_read_is_mark(sequence_type t) { return escape::next_to_read_is_mark(t); };
	void add_unjumpable_mark(sequence_type t) { escape::add_unjumpable_mark(t); };

	    // methods from generic_file redefined as protected

	virtual U_I inherited_read(char *a, U_I size) override;
	virtual void inherited_write(const char *a, U_I size) override;
	virtual void inherited_sync_write() override;
	    // inherited_flush_read() kept as is from the escape class
	    // inherited_terminate() kept as is from the escape class

    private:
	static bool initialized; ///< whether static field "zeroed_field" has been initialized
        static unsigned char zeroed_field[SPARSE_FIXED_ZEROED_BLOCK]; ///< read-only, used when the sequence of zeros is too short for a hole

	enum { normal, hole } mode; ///< wether we are currently reading/writing a hole or normal data
	infinint zero_count;     ///< number of zeroed byte pending in the current hole
	infinint offset;         ///< current offset in file (as if it was a plain file).
	infinint min_hole_size;  ///< minimum size of hole to consider
	U_I UI_min_hole_size;    ///< if possible store min_hole_size as U_I, if not this field is set to zero which disables the hole lookup inside buffers while writing data
	bool escape_write;       ///< whether to behave like an escape object when writing down data
	bool escape_read;        ///< whether to behave like an escape object when reading out data
	bool copy_to_no_skip;    ///< whether to hide holes by zeored bytes in the copy_to() methods
	bool seen_hole;          ///< whether a hole has been seen or this is a plain file so far
	bool data_escaped;       ///< whether some data has been escaped to not collide with a mark (may occur even when no hole is met)

	    /// write down the amount of byte zero not yet written.
	    /// which may be normal zeros or hole depending on their amount
	void dump_pending_zeros();

	    /// write a hole datastructure
	void write_hole(const infinint & length);

	    /// reset the sparse_file detection as if "this" object was just created
	    ///
	    /// \note may lead the offset to be backward it previous position
	void reset();


	    /// analyse a buffer for a hole

	    /// \param[in] a pointer to the buffer area
	    /// \param[in] size size of the buffer to inspect
	    /// \param[in] min_hole_size minimum size of hole to consider, if set to zero only consider hole at end of buffer
	    /// \param[out] offset in "a" where starts the found hole
	    /// \param[out] length length of the hole in byte
	    /// \return true if a hole has been found, false else
	    /// \note if the buffer ends by zeros, start points to the first zero, and length may be less than min_hole_size
	static bool look_for_hole(const char *a, U_I size, U_I min_hole_size, U_I & start, U_I & length);

	    /// count the number of zeroed byte starting at the provided buffer

	    /// \param[in] a a pointer to the buffer area
	    /// \param[in] size the size of the buffer to inspect
	    /// \return the number of zeroed bytes found at the beginning of the buffer
	static U_I count_initial_zeros(const char *a, U_I size);
    };

        /// @}

} // end of namespace


#endif

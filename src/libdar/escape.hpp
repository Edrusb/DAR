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
// $Id: escape.hpp,v 1.25 2011/04/17 18:43:39 edrusb Rel $
//
/*********************************************************************/

    /// \file escape.hpp
    /// \brief class escape definition, used for sequential reading of archives
    /// \ingroup Private
    ///
    /// The class escape is used to insert escape sequences before each new file's
    /// data in an archive. The normal file's data is also rewritten if it contains
    /// such an escape sequence for it does not collide with real escape sequences
    /// At reading time, this class revert backs modification done to file's data
    /// containing escape sequences for they contain the original data. This class
    /// also provides the feature to skip to the next (real) escape sequence.
    /// This class inherits of generic files and its objects are to be used in a
    /// stack of generic file's objects. The object below contains modified data
    /// and escape sequences, the file over gets the normal file data and does
    /// never see escape sequences. Expected implementation is to have a compressor
    /// above an escape object and a sar or scrambler/blowfish/... object above it.


#ifndef ESCAPE_HPP
#define ESCAPE_HPP

#include "../my_config.h"

extern "C"
{
#if HAVE_LIMITS_H
#include <limits.h>
#endif
}

#include <set>

#include "generic_file.hpp"

#define ESCAPE_FIXED_SEQUENCE_NORMAL 0xAD
#define ESCAPE_FIXED_SEQUENCE_SPARSE_FILE 0xAE

#define MAX_BUFFER_SIZE 204800
#ifdef SSIZE_MAX
#if SSIZE_MAX < MAX_BUFFER_SIZE
#undef MAX_BUFFER_SIZE
#define MAX_BUFFER_SIZE SSIZE_MAX
#endif
#endif

namespace libdar
{

	/// \addtogroup Private
	/// @{

    class escape : public generic_file
    {
    public:
	enum sequence_type
	{
	    seqt_undefined,       //< not enough data to define the type of the escape sequence
	    seqt_not_a_sequence,  //< to escape data corresponding to an escape sequence's fixed byte sequence
	    seqt_file,            //< placed before inode information, eventually followed by file data
	    seqt_ea,              //< placed before EA data
	    seqt_catalogue,       //< placed before the archive's internal catalogue
	    seqt_data_name,       //< placed before the archive data_name (at the beginning of the archive)
	    seqt_file_crc,        //< placed before the CRC of file's data
	    seqt_ea_crc,          //< placed before the CRC of file's EA
	    seqt_changed,         //< placed before new copy of file's data if file's data changed while reading it for backup
	    seqt_dirty,           //< placed after data CRC if file is dirty
	    seqt_failed_backup    //< placed after inode information if the file could not be openned at backup time
	};

	    // the archive layout of marks is :
	    // ... <seqt_file> <inode> [<file data> [<seqt_changed> <new copy of data> [...] ] <seqt_file_crc> <CRC>[<seqt_dirty>]] [<seqt_ea> <EA> <sqt_ea_crc> <CRC>] ...
	    // this previous sequence that we will call <SEQ> is repeated for each file, then on the overall archive we have :
	    // <seqt_data_name> <data_name> <SEQ> ... <SEQ> <seqt_catalogue> <catalogue> <terminator>

	    // the provided "below" object must exist during the whole live of the escape object, the escape object does not own this "below" object
	    // it must be destroyed by the caller/creator of the escape object.


	    // constructors & destructors

	escape(generic_file *below,                           //< "Below" is the generic file that holds the escaped data
	       const std::set<sequence_type> & x_unjumpable); //< a set of marks that can never been jumped over when skipping for the next mark of a any given type.
	escape(const escape & ref) : generic_file(ref) { copy_from(ref); };
	const escape & operator = (const escape & ref);
	~escape();

	    // escape specific routines

	void add_mark_at_current_position(sequence_type t);

	    /// skip forward to the next mark of given type

	    /// \param[in] t type of mark to skip to
	    /// \param[in] jump if set to false, do not jump over *any* mark, even those not set as unjumpable mark,
	    /// set it to true, to allow jumping on mark except those defined as unjumpable marks
	    /// \return true of could skip to mark of type t
	bool skip_to_next_mark(sequence_type t, bool jump);
	bool next_to_read_is_mark(sequence_type t);
	bool next_to_read_is_which_mark(sequence_type & t);

	void add_unjumpable_mark(sequence_type t) { if(is_terminated()) throw SRC_BUG; unjumpable.insert(t); };
	void remove_unjumpable_mark(sequence_type t);
	bool is_unjumpable_mark(sequence_type t) const { return unjumpable.find(t) != unjumpable.end(); };
	void clear_all_unjumpable_marks() { unjumpable.clear(); };

	    // generic_file inherited routines
	    // NOTA: Nothing is done to prevent skip* operation to put the read cursor in the middle of an escape sequence and
	    // thus incorrectly consider it as normal data. Such event should only occure upon archive corruption and will be detected
	    // by checksum mechanisms.

	bool skip(const infinint & pos);
	bool skip_to_eof();
	bool skip_relative(S_I x);
	infinint get_position();

	generic_file *get_below() const { return x_below; };

    protected:
	U_I inherited_read(char *a, U_I size);
	void inherited_write(const char *a, U_I size);
	void inherited_sync_write() { flush_write(); };
	void inherited_terminate() { flush_or_clean(); };

	void change_fixed_escape_sequence(unsigned char value) { fixed_sequence[0] = value; };
	bool has_escaped_data_since_last_skip() const { return escaped_data_count_since_last_skip > 0; };

    private:

	    //-- constants

	    /// total lenght of the escape sequence
	static const U_I ESCAPE_SEQUENCE_LENGTH = 6;
	static const U_I WRITE_BUFFER_SIZE = 2*ESCAPE_SEQUENCE_LENGTH;
	static const U_I READ_BUFFER_SIZE = MAX_BUFFER_SIZE;

	    /// escape sequence value

	    /// an escape sequence starts by this sequence of characters. The last one is replaced by the
	    /// type of sequence. The first one is fixed but may be set to another value using the protected method
	    /// change_escape_sequence(). This opens the possibility to have several nested escape objects
	    /// without having the bottom one escaping the escape sequence of the one above it.
	    /// this constant table is defined in escape.cpp
	static const unsigned char usual_fixed_sequence[ESCAPE_SEQUENCE_LENGTH];

	    //-- variables

	generic_file *x_below;        //< the generic_file in which we read/write escaped data from/to the object is not owned by "this"
	U_I write_buffer_size;        //< amount of data in write transit not yet written to "below" (may have to be escaped)
	char write_buffer[WRITE_BUFFER_SIZE]; //< data in write transit, all data is unescaped, up to the first real mark, after it, data is raw (may be escaped)
	                                      //< the first real mark is pointed to by escape_seq_offset_in_buffer
	U_I read_buffer_size;         //< amount of data in write transit, read from below, but not yet unescaped and returned to the upper layer
	U_I already_read;             //< data in buffer that has already returned to the upper layer
	bool read_eof;                //< whether we reached a escape sequence while reading data
	U_I escape_seq_offset_in_buffer; //< location of the first escape sequence which is not a data sequence
	char read_buffer[READ_BUFFER_SIZE]; //< data in read transit
	std::set<sequence_type> unjumpable; //< list of mark that cannot be jumped over when searching for the next mark
	unsigned char fixed_sequence[ESCAPE_SEQUENCE_LENGTH]; // the preambule of an escape sequence to use/search for
	infinint escaped_data_count_since_last_skip;

	    //-- routines

	void set_fixed_sequence_for(sequence_type t) { fixed_sequence[ESCAPE_SEQUENCE_LENGTH - 1] = type2char(t); };
	void check_below() { if(x_below == NULL) throw SRC_BUG; };
	void clean_data() { read_buffer_size = already_read = escape_seq_offset_in_buffer = 0; }; //< drops all in-transit data
	void flush_write();                                                                       //< write down to "below" all in-transit data
	void flush_or_clean()
	{
	    switch(get_mode())
	    {
	    case gf_read_only:
		clean_data();
		break;
	    case gf_write_only:
		flush_write();
		break;
	    default:
		throw SRC_BUG;
	    }
	};
	void copy_from(const escape & ref);
	bool mini_read_buffer(); //< returns true if it could end having at least ESCAPE_SEQUENCE_LENGTH bytes in read_buffer, false else (EOF reached).

	    //-- static routine(s)

	    // some convertion routines
	static char type2char(sequence_type x);
	static sequence_type char2type(char x);

	    /// unescape data from data marks, up to the first real escape sequence found

	    /// find the next start of escape sequence in the given buffer

	    /// \return the offset of the first start of escape sequence (ESCAPE_SEQUENCE_LENGTH - 1),
	    /// or partial if found at the end, returns size if none could be found
	static U_I trouve_amorce(const char *a, U_I size, const unsigned char escape_sequence[ESCAPE_SEQUENCE_LENGTH]);

	    /// unescape data from data marks, up to the first real escape sequence found

	    /// \param[in] a buffer to read data from
	    /// \param[in] size the amount of data in "a"
	    /// \param[out] delta the number of byte / the number of data escape sequence removed
	    /// \param[in] escape_sequence the escape sequence start to look for
	    /// return the offset of the real sequence and updates the size of the buffer
	    /// if some data mark have been removed (size gets smaller)
	static U_I remove_data_marks_and_stop_at_first_real_mark(char *a, U_I size, U_I & delta, const unsigned char escape_sequence[ESCAPE_SEQUENCE_LENGTH]);
    };

	/// @}

} // end of namespace

#endif

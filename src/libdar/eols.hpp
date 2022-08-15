/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2022 Denis Corbin
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
// to contact the author, see the AUTHOR file
/*********************************************************************/

    /// \file eols.hpp
    /// \brief provide a way to detect the presence of string from a list of string in a byte flow
    /// \ingroup Private
    ///
    /// This class is used by the mask_list class to detect the end of lines from
    /// a set of several variable length sequences provided by the user

#ifndef EOLS_HPP
#define EOLS_HPP

#include "../my_config.h"

#include <string>
#include <deque>

#include "integers.hpp"

namespace libdar
{

        /// \addtogroup API
        /// @{

        /// the class eols provide a way to detect the presence of string from a list of string in a byte flow

    class eols
    {
    public:
	eols(const std::deque<std::string> & end_sequences);

	eols(const eols & ref) { copy_from(ref); };
	eols(eols && ref) noexcept = default;
	eols & operator = (const eols & ref) { copy_from(ref); return *this; };
	eols & operator = (eols && ref) = default;
	~eols() = default;

	    /// add a new sequence for End of Line
	void add_sequence(const std::string & seq);

	    /// reset the detection to be beginning of each EOL sequence
	void reset_detection() const;

	    /// check whether we have reach an end of line

	    /// \param[in] next_read_byte the next byte read from the file
	    /// \param[out] eol_sequence_length if a EOL sequence is matched, the length of the sequence in bytes
	    /// \param[out] after_eol_read_bytes if a EOL sequence is matched, the amount of byte alread read after that sequence.
	    /// Important! the bytes read after eol must be re-fed to the eols object
	    /// \return true if a EOL sequence is matched, false else
	    /// \note due to the fact the list of sequence can contain sequence that is a substring of another
	    /// and that whe have to support the list { "\n" , "\n\r" } for backward compatibility, we need
	    /// to match the longest string. However it may end that a longer string than a one that just matched
	    /// finally does not match entierely, but to know that we would have read some extra bytes passed
	    /// the first (shorter but fully matched) sequence: for example with the two strings { "to", "toto" }
	    /// as EOL, the read sequence "totu" would have to read the 'u' byte to realize that the longest
	    /// sequence was 'to', thus we read the second 't' (and the 'u') passed the end of line to know that.
	bool eol_reached(char next_read_byte,
			 U_I & eol_sequence_length,
			 U_I & after_eol_read_bytes) const;

    private:
	class in_progress
	{
	public:
	    in_progress(const std::string & val): ref(val) { reset(); };

		// methods
	    void reset() const { next_to_match = ref.begin(); bypass = false; passed = 0; larger = false; };
	    bool match(char next_read_byte) const;
	    U_I progression() const; ///< returns the number of matching bytes so far
	    bool set_bypass(U_I prog) const; ///< set bypass if progression < prog or has_matched()
	    void set_larger() const { larger = true; };
	    bool has_matched() const { return next_to_match == ref.end(); };

		// fields
	    std::string ref;                                   ///< ending sequence
	    mutable std::string::const_iterator next_to_match; ///< next byte to read from that sequence (previously read bytes matched this sequence)
	    mutable bool bypass;                               ///< when set match() does nothing except counting bytes
	    mutable U_I passed;                                ///< number of bytes bypassed (read after end of sequence)
	    mutable bool larger;                               ///< another in_progress has matched we may also match as a larger sequence
	};

	std::deque<in_progress> eols_curs;
	mutable U_I ref_progression;

	    /// set bypass flag for all in_progress of eols_curs that progression is less than or equal 'prog'

	    /// \return true if all elements of eols_curs could be bypassed (none has a larger progression)
	bool bypass_or_larger(U_I prog) const;
	bool all_bypassed_or_matched() const;
	bool find_larger_match(U_I & seq_length, U_I & read_after_eol) const;
	void copy_from(const eols & ref);
    };

        /// @}

} // end of namespace

#endif

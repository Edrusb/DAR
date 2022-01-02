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

    /// \file header_flags.hpp
    /// \brief archive header flag field management
    /// \ingroup Private

#ifndef HEADER_FLAGS_HPP
#define HEADER_FLAGS_HPP

#include "../my_config.h"
#include "generic_file.hpp"

namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// manages the flag field used for now in the archive header

	/// \note historically flags were used a usual constant bitfield expression
	/// when 7 different bits were used came the question about adding more than one
	/// bit in the future. This last bit (0x01) has been reserved to flag that the
	/// bit field had a extra byte to take into account. By extension, the 0x0100 bit
	/// has been reserved to scale beyond 2 bytes or 14 different possible bits.
	/// In consequence, you cannot set or unset a bitfield having one of its byte
	/// with its least significant bit set (0x01, 0x0100, 0x1000,...).
	/// the use a meaning of each bit is left outside of this class to ease this
	/// class reuse in other context if the need arises in the future.
    class header_flags
    {
    public:
	header_flags(): bits(0) {};
	header_flags(generic_file & f) { read(f); };
	header_flags(const header_flags & ref) = default;
	header_flags(header_flags && ref) noexcept = default;
	header_flags & operator = (const header_flags & ref) = default;
	header_flags & operator = (header_flags && ref) noexcept = default;
	~header_flags() = default;

	    /// add all the bits set to 1 in the argument
	void set_bits(U_I bitfield);

	    /// remove all the bits set to in in the argument
	void unset_bits(U_I bitfield);

	    /// return true if *all* bits of the argument set to 1, are set in this header_flags
	bool is_set(U_I bitfield) const;

	    /// set the header_flags from a generic_file
	void read(generic_file & f);

	    /// dump the header_flags to generic_file
	void dump(generic_file & f) const;

	    /// clear all flags
	void clear() { bits = 0; };

	    /// whether all bits are cleared
	bool is_all_cleared() { return bits == 0; };

    private:
	U_I bits;    ///< future implementation could rely on infinint for a arbitrary large bitfield

	static bool has_an_lsb_set(U_I bitfield);
    };

} // end of namespace

#endif

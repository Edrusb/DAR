/*********************************************************************/
// dar - disk archive - a backup/restoration program
// Copyright (C) 2002-2023 Denis Corbin
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

    /// \file compress_block_header.hpp
    /// \brief block header used for compression per block
    /// \ingroup Private

#ifndef COMPRESS_BLOCK_HEADER_HPP
#define COMPRESS_BLOCK_HEADER_HPP

#include "../my_config.h"
#include "infinint.hpp"
#include "generic_file.hpp"


namespace libdar
{

	/// \addtogroup Private
	/// @{

	/// block header structure used for storing compressed blocks

	struct compress_block_header
	{
		// type values
	    static constexpr const char H_DATA = 1;
	    static constexpr const char H_EOF = 2;

		// fields

	    char type;             ///< let the possibility to extend this architecture (for now type is fixed)
	    infinint size;         ///< size of the following compressed block of data

	    void dump(generic_file & f);

		/// read the block header from a generic_file (an archive)

		/// \return true of a whole block header could be read.
		/// False is returned only if no data could be read at all (eof)
		/// else an exception is thrown
	    bool set_from(generic_file & f);
	};


	/// @}

} // end of namespace

#endif

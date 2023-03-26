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

    /// \file compression.hpp
    /// \brief compression parameters for API
    /// \ingroup API

#ifndef COMPRESSION_HPP
#define COMPRESSION_HPP

#include "../my_config.h"
#include <string>

namespace libdar
{

    	/// \addtogroup API
	/// @{

	/// the different compression algorithm available

	/// values to be used as argument of libdar API calls
	/// \note lzo1x_1_15 and lzo1x_1 should never be found in archive but instead lzo should
	/// be put in place. In consequence, the two letters 'j' and 'k' reserved here, shall
	/// well be modified to other value if necessary in the future, thus would not break
	/// any backward compatibility.
    enum class compression
    {
	none = 'n',        ///< no compression
	gzip = 'z',        ///< gzip compression (streamed)
	bzip2 = 'y',       ///< bzip2 compression (streamed)
	lzo = 'l',         ///< lzo compression (streamed)
	xz = 'x',          ///< lzma compression (streamed)
	lzo1x_1_15 = 'j',  ///< lzo degraded algo corresponding to lzop -1
        lzo1x_1 = 'k',     ///< lzo degraded algo corresponding to lzo -2 to lzo -6
        zstd = 'd',        ///< zstd compression
	lz4 = 'q',         ///< lz4 (streamed)
    };


	/// convert a char as stored in archive to its compression value
    extern compression char2compression(char a);

	/// return true if compression mode is per_block or false if
	/// this is a streamed compression
    extern bool char2compression_mode(char a);

	/// convert a compression value to a char for storing in archive
    extern char compression2char(compression c, bool per_block = false);

	/// convert a compression to its string representation
    extern std::string compression2string(compression c);

	/// convert a string representing a compression algorithm to its enum compression value
    extern compression string2compression(const std::string & a); // throw Erange if an unknown string is given

	/// @}

} // end of namespace

#endif
